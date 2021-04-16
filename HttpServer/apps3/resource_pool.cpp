#include "resource_pool.h"
#include "gdal_priv.h"

ResourcePool::ResourcePool()
{

}

ResourcePool::~ResourcePool()
{
	for (auto poDataset : dataset_pool_)
	{
		GDALClose(poDataset.first);
	}

	dataset_pool_.clear();
}

void ResourcePool::Init()
{
	dataset_pool_.reserve(dataset_pool_max_count_);
	for (int i = 0; i < dataset_pool_max_count_; i++)
	{
		GDALDataset* poDataset = (GDALDataset*)GDALOpen(path_.c_str(), GA_ReadOnly);
		dataset_pool_.push_back(std::make_pair(poDataset, false));
	}
}

void ResourcePool::SetDatasetPoolMaxCount(int count)
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	dataset_pool_max_count_ = count;
}

void ResourcePool::SetPath(const std::string& path)
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	path_ = path;
}

void ResourcePool::ClearDatasets()
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	dataset_pool_.clear();
}

GDALDataset* ResourcePool::Acquire()
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	for (auto& p : dataset_pool_)
	{
		if (p.second == false)
		{
			p.second = true;
			return p.first;
		}
	}

	return nullptr;
}

GDALDataset* ResourcePool::AcquireNoLock()
{
	for (auto& p : dataset_pool_)
	{
		if (p.second == false)
		{
			p.second = true;
			return p.first;
		}
	}

	return nullptr;
}

void ResourcePool::Release(GDALDataset* pd)
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	for (auto& p : dataset_pool_)
	{
		if (p.first == pd)
		{
			p.second = false;
			break;
		}
	}
}

bool ResourcePool::NeedReleaseDataset()
{
	std::lock_guard<std::mutex> guard(mutex_dataset_);
	for (auto& p : dataset_pool_)
	{
		if (p.second == true)
		{
			return true;
		}
	}

	return false;
}