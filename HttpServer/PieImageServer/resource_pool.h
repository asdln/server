#ifndef HTTPSERVER_RESOURCE_POOL_H_
#define HTTPSERVER_RESOURCE_POOL_H_

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include "dataset.h"

class ResourcePool
{
public:

	void Init();

	static ResourcePool* GetInstance()
	{
		{
			if (instance_ == nullptr)
			{
				std::lock_guard<std::mutex> guard(mutex_);
				{
					if (instance_ == nullptr)
					{
						instance_ = new ResourcePool;
					}
				}
			}

			return instance_;
		}
	}

	static void DestroyInstance();

	std::shared_ptr<Dataset> GetDataset(const std::string& path);

	OGRSpatialReference* GetSpatialReference(const std::string& wkt) { return nullptr; }

private:

	ResourcePool();
	virtual ~ResourcePool();
	ResourcePool(const ResourcePool&);
	ResourcePool& operator = (const ResourcePool&);


private:

	static ResourcePool* instance_;

	static std::mutex mutex_;

	int dataset_pool_max_count_ = 8;
	std::mutex mutex_dataset_;
	std::map<std::string, std::vector<DatasetPtr>> dataset_pool_;
};

#endif //HTTPSERVER_RESOURCE_POOL_H_