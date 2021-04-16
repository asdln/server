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
	volatile bool finished_ = false;

	//��һ�������ز���һ������Ƭ״̬��Ϣ����ʼ��Ϊ0
	std::atomic_char sub_tile_state_;

	//Ԥ�ڵ���Ƭ״̬��Ϣ���еĲ���4
	unsigned char sub_tile_state_expected = 4;

	//��һ����Ƭ��������Ϣ
	unsigned char sub_tile_arrange = 0x00;
};

class TaskRecord
{
public:

	TaskRecord(const std::string& region, const std::string& save_bucket_name
		, const std::string& aws_secret_access_key
		, const std::string& aws_access_key_id, int time_limit_sec = 780); // 13���ӣ�60 * 13 = 780

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

	size_t GetTileRecordsCount() { return count_; }

	int GetBandCount() { return band_count_; }

	GDALDataType GetDataType() { return type_; }

	size_t GetImgWidth() { return img_width_; }

	size_t GetImgHeight() { return img_height_; }

	bool IsReady();

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

	Aws::String src_bucket_name_;

	Aws::String src_key_name_;

	Aws::String aws_region_;

	Aws::String aws_secret_access_key_;

	Aws::String aws_access_key_id_;

	std::mutex mutex_global_;

	ResourcePool* resource_pool = nullptr;

	std::vector<std::pair<size_t, size_t>> dims_;

	TileRecord** tile_records_ = nullptr;
	size_t count_ = 0;

	size_t img_width_ = 0;
	size_t img_height_ = 0;
	int band_count_ = 3;
	GDALDataType type_ = GDT_Byte;

	//��ʼʱ���(����)
	long long start_sec_;

	int time_limit_sec_ = 780;
};

void DoTask(TaskRecord* task_record);

void ProcessLoop(TaskRecord* task_record, int& status);

bool AWSS3PutObject_File(const Aws::String& bucketName, const Aws::String& objectName
	, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, const std::string& file_name);