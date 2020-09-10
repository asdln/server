#include "resource_pool.h"
#include "dataset_factory.h"

ResourcePool* ResourcePool::instance_ = nullptr;
std::mutex ResourcePool::mutex_;

ResourcePool::ResourcePool()
{

}

ResourcePool::~ResourcePool()
{

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

std::shared_ptr<Dataset> ResourcePool::GetDataset(const std::string& path)
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);

	std::map<std::string, std::vector<DatasetPtr>>::iterator itr = dataset_pool_.find(path);
	if (itr != dataset_pool_.end())
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
		dataset_pool_.emplace(path, datasets);
		return p;
	}
}

HistogramPtr ResourcePool::GetHistogram(Dataset* tiff_dataset, const std::string& key, int band)
{
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

			histogram = ComputerHistogram(tiff_dataset, band);
			vec_histogram[band - 1] = histogram;
			map_histogram_.emplace(key, vec_histogram);
		}
		else
		{
			std::vector<HistogramPtr>& vec_histogram = itr->second;
			if (vec_histogram.size() < band)
			{
				vec_histogram.resize(band);
				histogram = ComputerHistogram(tiff_dataset, band);
				vec_histogram[band - 1] = histogram;
			}
			else
			{
				histogram = vec_histogram[band - 1];
				if (histogram == nullptr)
				{
					histogram = ComputerHistogram(tiff_dataset, band);
					vec_histogram[band - 1] = histogram;
				}
			}
		}

		return histogram;
	}
}