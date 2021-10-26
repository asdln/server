#include "resource_pool.h"
#include "dataset_factory.h"
#include "gdal_priv.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "etcd_storage.h"
#include "utility.h"

#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

ResourcePool* ResourcePool::instance_ = nullptr;
std::mutex ResourcePool::mutex_;
extern bool g_complete_statistic;

DatasetPtr DatasetGroup::wait()
{
	std::unique_lock<std::mutex> lock(mutex_dataset_group_);
	cond_.wait(lock, [&]() {return dataset_pool_max_count_ > 0; });
	--dataset_pool_max_count_;

	if (!datasets_.empty())
	{
		auto p = datasets_.back();
		datasets_.pop_back();
		return p;
	}
	else
	{
		auto p = DatasetFactory::OpenDataset(path_);
		if (p == nullptr)
		{
			++dataset_pool_max_count_;
		}

		return p;
	}
}

void DatasetGroup::signal(DatasetPtr dataset)
{
	std::unique_lock<std::mutex> lock(mutex_dataset_group_);
	++dataset_pool_max_count_;
	cond_.notify_one();

	datasets_.push_back(dataset);
}

ResourcePool::ResourcePool()
{

}

ResourcePool::~ResourcePool()
{
	map_SRS.clear();
	map_histogram_.clear();

	ClearDatasets();
}

void ResourcePool::Init()
{

}

void ResourcePool::DestroyInstance()
{
	if (instance_ != nullptr)
		delete instance_;

	instance_ = nullptr;
}

void ResourcePool::SetDatasetPoolMaxCount(int count)
{
	dataset_pool_max_count_ = count;

 	for (int i = 0; i < dataset_pool_max_count_; i++)
 	{
 		std::mutex* pmutex = new std::mutex;
 		list_cache_mutex_.push_back(pmutex);
 	}
}

std::mutex* ResourcePool::AcquireCacheMutex(const std::string& md5)
{
	std::lock_guard<std::mutex> guard(cache_mutex_);

	std::unordered_map<std::string, std::pair<std::mutex*, int>>::iterator itr = map_cache_mutex_.find(md5);
	if (itr != map_cache_mutex_.end())
	{
		itr->second.second++;
		return itr->second.first;
	}
	else
	{
		std::mutex* pmutex = list_cache_mutex_.back();
		list_cache_mutex_.pop_back();

		map_cache_mutex_.emplace(md5, std::make_pair(pmutex, 1));
		return pmutex;
	}
}

void ResourcePool::ReleaseCacheMutex(const std::string& md5)
{
	std::lock_guard<std::mutex> guard(cache_mutex_);
	std::unordered_map<std::string, std::pair<std::mutex*, int>>::iterator itr = map_cache_mutex_.find(md5);

	if (itr == map_cache_mutex_.end())
		return;

	itr->second.second--;

	if (itr->second.second == 0)
	{
		list_cache_mutex_.emplace_back(itr->second.first);
		map_cache_mutex_.erase(itr);
	}
}

std::shared_ptr<Dataset> ResourcePool::AcquireDataset(const std::string& path)
{
	//先用读锁
	std::shared_ptr<DatasetGroup> group = nullptr;
	{
		std::shared_lock<std::shared_mutex> lock(mutex_dataset_);
		auto itr = map_dataset_pool_.find(path);
		if (itr != map_dataset_pool_.end())
		{
			group = itr->second;
			//return itr->second->wait();
		}
	}

	if (group != nullptr)
	{
		return group->wait();
	}

	//没找到，用写锁
	{
		//需要再查找一次，因为另外的写锁可能抢先一步
		std::unique_lock<std::shared_mutex> lock(mutex_dataset_);
		auto itr = map_dataset_pool_.find(path);
		if (itr != map_dataset_pool_.end())
		{
			group = itr->second;
			//return itr->second->wait();
		}
		else
		{
			group = std::make_shared<DatasetGroup>(path, dataset_pool_max_count_);
			map_dataset_pool_.emplace(path, group);
		}
	}

	return group->wait();
}

void ResourcePool::ReleaseDataset(std::shared_ptr<Dataset> dataset)
{
	if (dataset == nullptr)
		return;

	//先用读锁
	std::shared_ptr<DatasetGroup> group = nullptr;
	{
		std::shared_lock<std::shared_mutex> lock(mutex_dataset_);
		auto itr = map_dataset_pool_.find(dataset->file_path());
		if (itr != map_dataset_pool_.end())
		{
			group = itr->second;
			//return itr->second->signal(dataset);
		}
	}

	if (group != nullptr)
	{
		return group->signal(dataset);
	}

	//没找到，用写锁
	{
		//需要再查找一次，因为另外的写锁可能抢先一步
		std::unique_lock<std::shared_mutex> lock(mutex_dataset_);
		auto itr = map_dataset_pool_.find(dataset->file_path());
		if (itr != map_dataset_pool_.end())
		{
			//return itr->second->signal(dataset);
			group = itr->second;
		}
		else
		{
			group = std::make_shared<DatasetGroup>(dataset->file_path(), dataset_pool_max_count_);
			map_dataset_pool_.emplace(dataset->file_path(), group);
		}
	}

	return group->signal(dataset);
}

void ResourcePool::ClearDatasets()
{
	std::unique_lock<std::shared_mutex> lock(mutex_dataset_);

	for (auto itr : map_dataset_pool_)
	{
		itr.second->wait();
	}
	
	map_dataset_pool_.clear();
}

OGRSpatialReference* ResourcePool::GetSpatialReference(int epsg_code)
{
	{
		std::shared_lock<std::shared_mutex> lock(srs_shared_mutex_);

		std::map<int, std::shared_ptr<OGRSpatialReference>>::iterator itr = map_SRS.find(epsg_code);
		if (itr != map_SRS.end())
		{
			return itr->second.get();
		}
	}

	{
		std::unique_lock<std::shared_mutex> lock(srs_shared_mutex_);
		std::map<int, std::shared_ptr<OGRSpatialReference>>::iterator itr = map_SRS.find(epsg_code);
		if (itr != map_SRS.end())
		{
			return itr->second.get();
		}
		else
		{
			auto srs = std::make_shared<OGRSpatialReference>();
			srs->importFromEPSG(epsg_code);
			map_SRS.emplace(epsg_code, srs);
			return srs.get();
		}
	}

	return nullptr;
}

std::vector<HistogramPtr> ReadAndSaveHistogramFile(Dataset* dataset, bool have_nodata_value_, double nodata_value_, std::string& histogram_content)
{
 	int band_count = dataset->GetBandCount();
	Histogram_ContainerSTL container;

 	if (dataset->ReadHistogramFile(histogram_content))
 	{
 		std::cout << "read histogram from file" << std::endl;
 
 		std::istringstream is(histogram_content);
 		boost::archive::binary_iarchive ia(is);
 		ia >> container;
 	}
 	else
 	{
 		std::cout << "begin compute histogram ..." << std::endl;
 		
 
 		for (int i = 0; i < band_count; i++)
 		{
 			HistogramPtr histogram = ComputerHistogram(dataset, i + 1, false/*g_complete_statistic*/, have_nodata_value_, nodata_value_);
 			container.histograms.push_back(histogram);
 		}

 		std::ostringstream os;
 		boost::archive::binary_oarchive oa(os);
 		oa << container;
 
 		histogram_content = os.str();
 		dataset->SaveHistogramFile(histogram_content);
 
 		std::cout << "end compute histogram ..." << std::endl;
 	}

	return container.histograms;
}

std::vector<HistogramPtr> GetHistogramFromEtcdOrCalc(const std::string& key, Dataset* tiff_dataset, bool use_external_no_data, double external_no_data_value)
{
	//如果使用etcd，则先从etcd获取
	EtcdStorage etcd_storage;
	std::vector<HistogramPtr> res;

	if (etcd_storage.IsUseEtcd())
	{
		std::string histogram_key = key + "_histogram";
		std::string histogram_lock = key + "_lock";
		if (etcd_storage.Lock(histogram_lock))
		{
			std::string histograms_str;
			if (etcd_storage.GetValue(histogram_key, histograms_str))
			{
				Histogram_ContainerSTL container;
				std::istringstream is(histograms_str);
				boost::archive::binary_iarchive ia(is);
				ia >> container;

				res = container.histograms;

				std::cout << "ln_debug: get histogram info from etcd" << std::endl;
			}
			else
			{
				std::string histogram_content;
				res = ReadAndSaveHistogramFile(tiff_dataset, use_external_no_data, external_no_data_value, histogram_content);
				etcd_storage.SetValue(histogram_key, histogram_content, true, 3600); //ttl 设为 一小时
			}

			etcd_storage.Unlock(histogram_lock);
		}
	}
	else
	{
		std::string histogram_content;
		res = ReadAndSaveHistogramFile(tiff_dataset, use_external_no_data, external_no_data_value, histogram_content);
	}

	return res;
}

HistogramPtr ResourcePool::GetHistogram(Dataset* tiff_dataset, int band, bool use_external_no_data, double external_no_data_value)
{
	std::string key = tiff_dataset->file_path();
	key = ReplaceStringInStd(key, "/", "_");

	if (use_external_no_data)
	{
		std::string no_data_str = boost::lexical_cast<std::string>(external_no_data_value);
		key += no_data_str;
	}
	
	{
		//先用读锁，看能不能获取到，如果获取不到，再用写锁
		std::shared_lock<std::shared_mutex> lock(shared_mutex_);
		HistogramPtr histogram;
		std::map<std::string, std::vector<HistogramPtr>>::iterator itr = map_histogram_.find(key);
		if (itr != map_histogram_.end() && itr->second.size() >= band && itr->second[band - 1] != nullptr)
		{
			histogram = itr->second[band - 1];
			return histogram;
		}
	}

	{
		//没有获取到，使用写锁。
		std::unique_lock<std::shared_mutex> lock(shared_mutex_);
		HistogramPtr histogram;
		std::map<std::string, std::vector<HistogramPtr>>::iterator itr = map_histogram_.find(key);
		if (itr == map_histogram_.end())
		{
			std::vector<HistogramPtr> vec_histogram = GetHistogramFromEtcdOrCalc(key, tiff_dataset, use_external_no_data, external_no_data_value);
			map_histogram_.emplace(key, vec_histogram);
			histogram = vec_histogram[band - 1];
		}
		else
		{
			std::vector<HistogramPtr>& vec_histogram = itr->second;
			histogram = vec_histogram[band - 1];
		}

		return histogram;
	}
}