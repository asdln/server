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
	auto p = DatasetFactory::OpenDataset(path);
	return p;
}