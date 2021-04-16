#ifndef HTTPSERVER_RESOURCE_POOL_H_
#define HTTPSERVER_RESOURCE_POOL_H_

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <shared_mutex>
#include "gdal_priv.h"

class ResourcePool
{
public:

	ResourcePool();
	virtual ~ResourcePool();

	void Init();

	void SetPath(const std::string& path);

	GDALDataset* Acquire();

	GDALDataset* AcquireNoLock();

	void Lock() { mutex_dataset_.lock(); }

	void Unlock() { mutex_dataset_.unlock(); }

	void Release(GDALDataset* pd);

	void ClearDatasets();

	void SetDatasetPoolMaxCount(int count);

	bool NeedReleaseDataset();

private:

	std::string path_;

	int dataset_pool_max_count_ = 8;
	std::mutex mutex_dataset_;
	std::vector<std::pair<GDALDataset*, bool>> dataset_pool_;
};

#endif //HTTPSERVER_RESOURCE_POOL_H_