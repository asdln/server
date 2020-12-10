#include "resource_pool.h"
#include "dataset_factory.h"
#include "gdal_priv.h"
#include <boost/lexical_cast.hpp>

ResourcePool* ResourcePool::instance_ = nullptr;
std::mutex ResourcePool::mutex_;
extern bool g_complete_statistic;

ResourcePool::ResourcePool()
{

}

ResourcePool::~ResourcePool()
{
	map_SRS.clear();
	map_histogram_.clear();
	map_dataset_pool_.clear();
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
}

std::shared_ptr<Dataset> ResourcePool::GetDataset(const std::string& path)
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);

	std::map<std::string, std::vector<DatasetPtr>>::iterator itr = map_dataset_pool_.find(path);
	if (itr != map_dataset_pool_.end())
	{
		std::vector<DatasetPtr>& datasets = itr->second;
		if (datasets.size() < dataset_pool_max_count_)
		{
			auto p = DatasetFactory::OpenDataset(path);
			datasets.emplace_back(p);
			return p;
		}
		else
		{
			int use_count = -1;
			DatasetPtr min_count_dataset;
			for (auto dataset : datasets)
			{
				if (-1 == use_count)
				{
					use_count = dataset.use_count();
					min_count_dataset = dataset;
				}
				else
				{
					if (use_count > dataset.use_count())
					{
						use_count = dataset.use_count();
						min_count_dataset = dataset;
					}
				}
			}
			return min_count_dataset;
		}
	}
	else
	{
		std::vector<DatasetPtr> datasets;
		auto p = DatasetFactory::OpenDataset(path);
		datasets.emplace_back(p);
		map_dataset_pool_.emplace(path, datasets);
		return p;
	}
}

void ResourcePool::ClearDatasets()
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
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

HistogramPtr ResourcePool::GetHistogram(Dataset* tiff_dataset, int band, bool use_external_no_data, double external_no_data_value)
{
	std::string key = tiff_dataset->file_path();

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
			std::vector<HistogramPtr> vec_histogram;
			vec_histogram.resize(band);

			histogram = ComputerHistogram(tiff_dataset, band, g_complete_statistic, use_external_no_data, external_no_data_value);
			vec_histogram[band - 1] = histogram;
			map_histogram_.emplace(key, vec_histogram);
		}
		else
		{
			std::vector<HistogramPtr>& vec_histogram = itr->second;
			if (vec_histogram.size() < band)
			{
				vec_histogram.resize(band);
				histogram = ComputerHistogram(tiff_dataset, band, g_complete_statistic, use_external_no_data, external_no_data_value);
				vec_histogram[band - 1] = histogram;
			}
			else
			{
				histogram = vec_histogram[band - 1];
				if (histogram == nullptr)
				{
					histogram = ComputerHistogram(tiff_dataset, band, g_complete_statistic, use_external_no_data, external_no_data_value);
					vec_histogram[band - 1] = histogram;
				}
			}
		}

		return histogram;
	}
}