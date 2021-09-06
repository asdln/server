#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <shared_mutex>
#include "dataset.h"

class DatasetGroup
{
public:

	DatasetGroup(const std::string& path, int max_count) 
		: path_(path), dataset_pool_max_count_(max_count){}

	DatasetPtr wait();

	void signal(DatasetPtr);

protected:

	std::string path_;

	std::vector<DatasetPtr> datasets_;

	std::condition_variable cond_;
	int dataset_pool_max_count_;
	std::mutex mutex_dataset_group_;
};

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

	std::mutex* AcquireCacheMutex(const std::string& md5);

	void ReleaseCacheMutex(const std::string& md5);

	std::shared_ptr<Dataset> AcquireDataset(const std::string& path);

	void ReleaseDataset(std::shared_ptr<Dataset> dataset);

	OGRSpatialReference* GetSpatialReference(int epsg_code);

	void ClearDatasets();

	//use_external_no_data 是否使用外部传入的无效值。有些影像本身没有无效值信息，但是有黑边。外部传入无效值，去黑边
	HistogramPtr GetHistogram(Dataset* tiff_dataset, int band, bool use_external_no_data, double external_no_data_value);

	void SetDatasetPoolMaxCount(int count);

private:

	ResourcePool();
	virtual ~ResourcePool();
	ResourcePool(const ResourcePool&);
	ResourcePool& operator = (const ResourcePool&);


private:

	std::mutex cache_mutex_;
	std::list<std::mutex*> list_cache_mutex_;
	std::unordered_map<std::string, std::pair<std::mutex*, int>> map_cache_mutex_;

	static ResourcePool* instance_;

	int dataset_pool_max_count_ = 8;
	static std::mutex mutex_;
	std::shared_mutex mutex_dataset_;
	std::map<std::string, std::shared_ptr<DatasetGroup>> map_dataset_pool_;

	std::shared_mutex srs_shared_mutex_;
	std::map<int, std::shared_ptr<OGRSpatialReference>> map_SRS;

	std::shared_mutex shared_mutex_;
	std::map<std::string, std::vector<HistogramPtr>> map_histogram_;
};