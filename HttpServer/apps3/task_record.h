#pragma once

#include <atomic>
#include "gdal_priv.h"
#include "resource_pool.h"
#include <aws/core/Aws.h>

const int code_success = 0;
const int code_fail = -1;
const int code_not_finished = 1;

struct TileRecord
{
	TileRecord(size_t col, size_t row, size_t z)
	{
		col_ = col;
		row_ = row;
		z_ = z;

		sub_tile_state_ = 0;
	}

	//瓦片的索引
	size_t col_;
	size_t row_;
	size_t z_;

	//数据集。只在z_ = 0 的级别复制，并且只在z_ = 1 的级别使用
	GDALDataset* poDataset_ = nullptr;

	//datast是否被占用
	std::mutex dataset_lock_flag_mutex_;

	//当前瓦片是否正在处理
	std::mutex processing_mutex_;

	//当前瓦片是否已经处理完毕
	volatile bool finished_ = false;

	//下一级（被重采样一级）瓦片状态信息。初始化为0
	std::atomic_char sub_tile_state_;

	//预期的瓦片状态信息。有的不是4
	unsigned char sub_tile_state_expected_ = 4;

	//下一级瓦片的排列信息
	unsigned char sub_tile_arrange_ = 0x00;
};

class TaskRecord
{
public:

	TaskRecord(const std::string& region, const std::string& save_bucket_name
		, const std::string& aws_secret_access_key
		, const std::string& aws_access_key_id, int time_limit_sec = 780); // 13分钟，60 * 13 = 780

	~TaskRecord();

	bool Open(const std::string& path, int dataset_count);

	std::string ToJson();

	bool FromJson(const std::string& json);

	void Lock() { resource_pool->Lock(); }

	void Unlock() { resource_pool->Unlock(); }

	GDALDataset* AcquireNoLock() { return resource_pool->AcquireNoLock(); }

	GDALDataset* Acquire() { return resource_pool->Acquire(); }

	TileRecord* GetTileRecord(size_t index) { return tile_records_[index]; }

	TileRecord* GetTileRecord(size_t col, size_t row, size_t z);

	size_t GetTileRecordsCount() { return tile_records_.size(); }

	int GetBandCount() { return band_count_; }

	GDALDataType GetDataType() { return type_; }

	size_t GetImgWidth() { return img_width_; }

	size_t GetImgHeight() { return img_height_; }

	bool IsReady();

	void GetTileStatusCount(size_t& finished, size_t& unfinished);

	bool NeedReleaseDataset();

	void ReleaseDataset();

	std::mutex& GetMutex() { return mutex_global_; }

	std::string& GetPath() { return path_; }

	const Aws::String& GetRegion() { return aws_region_; }

	const Aws::String& GetSrcBucket() { return src_bucket_name_; }

	const Aws::String& GetSaveBucket() { return save_bucket_name_; }

	const Aws::String& GetSrcKey() { return src_key_name_; }

	const Aws::String& GetAccessKeyID() { return aws_access_key_id_; }

	const Aws::String& GetSecretAccessKey() { return aws_secret_access_key_; }

	long long GetStartSec() { return start_sec_; }

	int GetTimeLimitSec() { return time_limit_sec_; }

protected:

	Aws::String save_bucket_name_;

	std::string path_;

	std::string format_;

	std::string info_json_path_;

	Aws::String src_bucket_name_;

	Aws::String src_key_name_;

	Aws::String aws_region_;

	Aws::String aws_secret_access_key_;

	Aws::String aws_access_key_id_;

	std::mutex mutex_global_;

	ResourcePool* resource_pool = nullptr;

	std::vector<std::pair<size_t, size_t>> dims_;

	std::vector<TileRecord*> tile_records_;
	//TileRecord** tile_records_ = nullptr;

	size_t img_width_ = 0;
	size_t img_height_ = 0;
	int band_count_ = 3;
	GDALDataType type_ = GDT_Byte;

	//初始时间戳(到秒)
	long long start_sec_;

	int time_limit_sec_ = 780;
};

void DoTask(TaskRecord* task_record);

void ProcessLoop(TaskRecord* task_record, int& status);

bool AWSS3PutObject_File(const Aws::String& bucketName, const Aws::String& objectName
	, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, const std::string& file_name);

bool AWSS3DeleteObject(const Aws::String& objectKey,
	const Aws::String& fromBucket, const Aws::String& region
	, const Aws::String& aws_secret_access_key, const Aws::String& aws_access_key_id);