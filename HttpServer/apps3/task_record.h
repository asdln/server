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

	//��Ƭ������
	size_t col_;
	size_t row_;
	size_t z_;

	//���ݼ���ֻ��z_ = 0 �ļ����ƣ�����ֻ��z_ = 1 �ļ���ʹ��
	GDALDataset* poDataset_ = nullptr;

	//datast�Ƿ�ռ��
	std::mutex dataset_lock_flag_mutex_;

	//��ǰ��Ƭ�Ƿ����ڴ���
	std::mutex processing_mutex_;

	//��ǰ��Ƭ�Ƿ��Ѿ��������
	std::atomic_char finished_ = {0};

	//��һ�������ز���һ������Ƭ״̬��Ϣ����ʼ��Ϊ0
	std::atomic_char sub_tile_state_;

	//Ԥ�ڵ���Ƭ״̬��Ϣ���еĲ���4
	unsigned char sub_tile_state_expected_ = 4;

	//��һ����Ƭ��������Ϣ
	unsigned char sub_tile_arrange_ = 0x00;
};

class TaskRecord
{
public:

	TaskRecord(const std::string& region, const std::string& save_bucket_name
		, const std::string& aws_secret_access_key
		, const std::string& aws_access_key_id, int time_limit_sec = 780, int force = 0); // 13���ӣ�60 * 13 = 780

	~TaskRecord();

	bool Open(const std::string& path, int dataset_count);

	void DoStatistic();

	std::string ToJson();

	bool FromJson(const std::string& json);

// 	void Lock() { resource_pool->Lock(); }
// 
// 	void Unlock() { resource_pool->Unlock(); }
// 
// 	GDALDataset* AcquireNoLock() { return resource_pool->AcquireNoLock(); }
// 
// 	GDALDataset* Acquire() { return resource_pool->Acquire(); }

	TileRecord* GetTileRecord(size_t index) { return tile_records_[index]; }

	TileRecord* GetTileRecord(size_t col, size_t row, size_t z);

	size_t GetTileRecordsCount() { return tile_records_.size(); }

	int GetBandCount() { return band_count_; }

	GDALDataType GetDataType() { return type_; }

	size_t GetImgWidth() { return img_width_; }

	size_t GetImgHeight() { return img_height_; }

	bool IsReady();

	void GetTileStatusCount(size_t& finished, size_t& unfinished);

// 	bool NeedReleaseDataset();
// 
// 	void ReleaseDataset();

	std::mutex& GetMutex() { return mutex_global_; }

	std::string& GetPath() { return path_; }

	double GetNoDataValue(int& pbSuccess) { pbSuccess = have_nodata_value_; return nodata_value_; }

	const Aws::String& GetRegion() { return aws_region_; }

	const Aws::String& GetSaveBucket() { return save_bucket_name_; }

	const Aws::String& GetSaveKey() { return save_key_name_; }

	const Aws::String& GetAccessKeyID() { return aws_access_key_id_; }

	const Aws::String& GetSecretAccessKey() { return aws_secret_access_key_; }

	long long GetStartSec() { return start_sec_; }

	int GetTimeLimitSec() { return time_limit_sec_; }

	void GetDims(int z, int& cols, int& rows);

protected:

	Aws::String save_bucket_name_;

	Aws::String save_key_name_;

	std::string save_bucket_key_;

	std::string path_;

	int force_ = 0;

	std::string format_;

	std::string info_json_path_;

	Aws::String aws_region_;

	Aws::String aws_secret_access_key_;

	Aws::String aws_access_key_id_;

	std::mutex mutex_global_;

	//ResourcePool* resource_pool = nullptr;

	std::vector<std::pair<size_t, size_t>> dims_;

	std::vector<TileRecord*> tile_records_;
	//TileRecord** tile_records_ = nullptr;

	int have_nodata_value_ = 0;
	double nodata_value_ = 0.0;
	size_t img_width_ = 0;
	size_t img_height_ = 0;
	int band_count_ = 3;
	GDALDataType type_ = GDT_Byte;

	//��ʼʱ���(����)
	long long start_sec_;

	int time_limit_sec_ = 780;
};

void ProcessLoop(TaskRecord* task_record, int& status);

bool AWSS3PutObject_File(const Aws::String& bucketName, const Aws::String& objectName
	, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, const std::string& file_name);

bool AWSS3DeleteObject(const Aws::String& objectKey,
	const Aws::String& fromBucket, const Aws::String& region
	, const Aws::String& aws_secret_access_key, const Aws::String& aws_access_key_id);