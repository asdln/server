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