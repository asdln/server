#include "task_record.h"
#include "resource_pool.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#ifdef USE_FILE
#include "jpg_compress.h"
#include <filesystem>
#endif

#include <random>
#include "CJsonObject.hpp"

#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/auth/AWSCredentials.h>


bool AWSS3GetObject(const Aws::String& fromBucket , const Aws::String& objectKey,
	const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, std::vector<unsigned char>& buffer)
{
	Aws::Client::ClientConfiguration config;

	if (!region.empty())
	{
		config.region = region;
	}

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	Aws::S3::S3Client s3_client(cred, config);
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.SetBucket(fromBucket);
	object_request.SetKey(objectKey);

	Aws::S3::Model::GetObjectOutcome get_object_outcome =
		s3_client.GetObject(object_request);

	if (get_object_outcome.IsSuccess())
	{
		auto& retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
		size_t content_bytes = get_object_outcome.GetResultWithOwnership().GetContentLength();

		retrieved_file.read((char*)buffer.data(), content_bytes);
		return true;
	}
	else
	{
		auto err = get_object_outcome.GetError();
		std::cout << "Error: GetObject: " <<
			err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

		return false;
	}
}

bool AWSS3PutObject(const Aws::String& bucketName, const Aws::String& objectName
	, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, std::vector<unsigned char>& buffer)
{
	Aws::Client::ClientConfiguration config;

	if (!region.empty())
	{
		config.region = region;
	}

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	Aws::S3::S3Client s3_client(cred, config);

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(bucketName);
	request.SetKey(objectName);

	auto sbuff = Aws::New<Aws::Utils::Stream::PreallocatedStreamBuf>("whatever string",
		buffer.data(), buffer.size());

	auto sbody = Aws::MakeShared<Aws::IOStream>("whatever string", sbuff);
	request.SetBody(sbody);

	Aws::S3::Model::PutObjectOutcome outcome =
		s3_client.PutObject(request);

	if (outcome.IsSuccess()) {

		//std::cout << "Added object '" << objectName << "' to bucket '"
		//	<< bucketName << "'.";
		return true;
	}
	else
	{
		std::cout << "Error: PutObject: " <<
			outcome.GetError().GetMessage() << std::endl;

		return false;
	}
}

bool AWSS3PutObject_File(const Aws::String& bucketName, const Aws::String& objectName
	, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, const std::string& file_name)
{
	Aws::Client::ClientConfiguration config;

	if (!region.empty())
	{
		config.region = region;
	}

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	Aws::S3::S3Client s3_client(cred, config);

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(bucketName);
	request.SetKey(objectName);

	auto input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
			file_name.c_str(), std::ios_base::in | std::ios_base::binary);
	request.SetBody(input_data);

	Aws::S3::Model::PutObjectOutcome outcome =
		s3_client.PutObject(request);

	if (outcome.IsSuccess()) {

		std::cout << "Added object '" << objectName << "' to bucket '"
			<< bucketName << "'.";
		return true;
	}
	else
	{
		std::cout << "Error: PutObject: " <<
			outcome.GetError().GetMessage() << std::endl;

		return false;
	}
}

TaskRecord::TaskRecord(const std::string& region, const std::string& save_bucket_name
	, const std::string& aws_secret_access_key
	, const std::string& aws_access_key_id, int time_limit_sec) : time_limit_sec_(time_limit_sec)
{
	aws_region_ = Aws::String(region.c_str(), region.size());
	save_bucket_name_ = Aws::String(save_bucket_name.c_str(), save_bucket_name.size());

	aws_access_key_id_ = Aws::String(aws_access_key_id.c_str(), aws_access_key_id.size());
	aws_secret_access_key_ = Aws::String(aws_secret_access_key.c_str(), aws_secret_access_key.size());

	std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
	start_sec_ = std::chrono::duration_cast<std::chrono::seconds>(d).count();
}

TaskRecord::~TaskRecord()
{
	for (size_t i = 0; i < count_; i++)
	{
		delete tile_records_[i];
	}

	delete[] tile_records_;
}

bool TaskRecord::Open(const std::string& path, int dataset_count)
{
	path_ = path;

	std::string dir = path + ".pyra";
#ifdef USE_FILE
	if (!std::filesystem::exists(dir))
	{
		if (!std::filesystem::create_directories(dir))
		{
			std::cout << "create path failed: " << path << std::endl;
		}
	}
#else

	std::string temp_str = path;
	if (temp_str.rfind("/vsis3/", 0) != 0)
	{
		std::cout << "error: path not begin with \"/vsis3/\"" << std::endl;
		return false;
	}

	temp_str = std::string(temp_str.c_str() + 7, temp_str.size() - 7);
	size_t pos = temp_str.find("/");

	src_bucket_name_ = Aws::String(temp_str.c_str(), pos);

	src_key_name_ = Aws::String(temp_str.c_str() + pos + 1, temp_str.size() - pos - 1);

#endif // USE_FILE


	GDALAllRegister();
	//size_t img_width, size_t img_height, int band_count, GDALDataType type
	GDALDataset* poDataset = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
	if (poDataset == nullptr)
	{
		std::cout << "error: GDALOpen failed" << std::endl;
		return false;
	}

	img_width_ = poDataset->GetRasterXSize();
	img_height_ = poDataset->GetRasterYSize();
	band_count_ = poDataset->GetRasterCount();
	type_ = poDataset->GetRasterBand(1)->GetRasterDataType();
	GDALClose(poDataset);
	poDataset = nullptr;

	resource_pool = new ResourcePool;
	resource_pool->SetPath(path);
	resource_pool->SetDatasetPoolMaxCount(dataset_count);
	resource_pool->Init();

	int size = 256;

	while (true)
	{
		size *= 2;
		size_t tile_col = img_width_ % size == 0 ? img_width_ / size : img_width_ / size + 1;
		size_t tile_row = img_height_ % size == 0 ? img_height_ / size : img_height_ / size + 1;

		if (tile_col == 0)
			tile_col = 1;

		if (tile_row == 0)
			tile_row = 1;

		dims_.push_back(std::make_pair(tile_col, tile_row));

		for (size_t j = 0; j < tile_row; j++)
		{
			for (size_t i = 0; i < tile_col; i++)
			{
				count_++;
			}
		}

		if (tile_col == 1 && tile_row == 1)
			break;
	}

	std::cout << "tile count : " << count_ << std::endl;
	tile_records_ = new TileRecord * [count_];

	size = 256;
	size_t index = 0;
	size_t count = 0;
	while (true)
	{
		index++;
		size *= 2;
		size_t tile_col = img_width_ % size == 0 ? img_width_ / size : img_width_ / size + 1;
		size_t tile_row = img_height_ % size == 0 ? img_height_ / size : img_height_ / size + 1;

		if (tile_col == 0)
			tile_col = 1;

		if (tile_row == 0)
			tile_row = 1;

		for (size_t j = 0; j < tile_row; j++)
		{
			for (size_t i = 0; i < tile_col; i++)
			{
				TileRecord* tile_record = new TileRecord(i, j, index);
				tile_records_[count++] = tile_record;

				if (j == 0 && i == 0 && index == 2)
				{
					int x = 0;
				}

				//如果是1级，则数据已经准备好了(直接读原始数据)
				if (index == 1)
				{
					//默认全部就绪。边角的地方，在读取时做特殊处理。
					tile_record->sub_tile_state_expected = 4;
					tile_record->sub_tile_arrange = 0x0f;
					tile_record->sub_tile_state_ = 4;
				}
				else
				{
					size_t cols = dims_[index - 2].first;
					size_t rows = dims_[index - 2].second;

					long long ii = (i + 1) * 2 - cols;
					long long jj = (j + 1) * 2 - rows;

					if (ii < 0)
						ii = 0;

					if (jj < 0)
						jj = 0;

					if (ii == 0 && jj == 0)
					{
						tile_record->sub_tile_state_expected = 4;
						tile_record->sub_tile_arrange = 0x0f; //1111
					}
					else if (ii == 1 && jj == 1)
					{
						tile_record->sub_tile_state_expected = 1;
						tile_record->sub_tile_arrange = 0x08; // 1000
					}
					else if (ii == 0 && jj == 1)
					{
						tile_record->sub_tile_state_expected = 2;
						tile_record->sub_tile_arrange = 0x0c; //1100
					}
					else if (ii == 1 && jj == 0)
					{
						tile_record->sub_tile_state_expected = 2;
						tile_record->sub_tile_arrange = 0x0a; //1010
					}
				}
			}
		}

		if (tile_col == 1 && tile_row == 1)
			break;
	}

	std::string info_json_path = dir + "/info.json";

	std::ifstream inFile2(info_json_path, std::ios::binary | std::ios::in);
	if (inFile2.is_open())
	{
		inFile2.seekg(0, std::ios_base::end);
		int FileSize2 = inFile2.tellg();

		inFile2.seekg(0, std::ios::beg);

		std::vector<unsigned char> buffer;
		//char* buffer2 = new char[FileSize2];
		buffer.resize(FileSize2);
		inFile2.read((char*)buffer.data(), FileSize2);
		inFile2.close();

		std::string string_json(buffer.begin(), buffer.end());

		if (!FromJson(string_json))
			return false;

		std::cout << info_json_path << " loaded" << std::endl;
	}
	
	return true;
}

std::string TaskRecord::ToJson()
{
	neb::CJsonObject oJson;
	oJson.Add("count", count_);
	oJson.AddEmptySubArray("tile_finished");
	oJson.AddEmptySubArray("sub_tile_state");

	for (int i = 0; i < count_; i++)
	{
		TileRecord* tile_record = tile_records_[i];

		oJson["tile_finished"].Add(tile_record->finished_);
		oJson["sub_tile_state"].Add(tile_record->sub_tile_state_);
	}

	return oJson.ToString();
}

bool TaskRecord::FromJson(const std::string& json)
{
	if (json.empty())
		return false;

	neb::CJsonObject oJson(json);

	int count;
	if (oJson.Get("count", count))
		return false;

	if (count != count_)
		return false;

	for (int i = 0; i < count_; i++)
	{
		TileRecord* tile_record = tile_records_[i];

		bool tile_finished;
		oJson["tile_finished"].Get(i, tile_finished);
		tile_record->finished_ = tile_finished;

		int sub_tile_state;
		oJson["sub_tile_state"].Get(i, sub_tile_state);
		tile_record->sub_tile_state_ = sub_tile_state;
	}

	return true;
}

TileRecord* TaskRecord::GetTileRecord(size_t col, size_t row, size_t z)
{
	if (z - 1 >= dims_.size())
		return nullptr;

	size_t index = 0;
	for (int i = 0; i < z - 1; i++)
	{
		size_t cols = dims_[i].first;
		size_t rows = dims_[i].second;

		index += cols * rows;
	}

	index += dims_[z - 1].first * row + col;

	TileRecord* res = tile_records_[index];
	if (res->col_ != col || res->row_ != row || res->z_ != z)
		return nullptr;

	return res;
}

bool TaskRecord::IsReady()
{
	for (size_t i = 0; i < count_; i++)
	{
		if (tile_records_[i]->finished_ == false)
			return false;
	}

	return true;
}

bool TaskRecord::NeedReleaseDataset()
{
	return resource_pool->NeedReleaseDataset();
}

void TaskRecord::ReleaseDataset()
{
	if (!NeedReleaseDataset())
		return;

	std::lock_guard<std::mutex> guard(mutex_global_);

	size_t cols = dims_[0].first;
	size_t rows = dims_[0].second;

	for (size_t j = 0; j < rows; j++)
	{
		size_t finished_count = 0;
		for (size_t i = 0; i < cols; i++)
		{
			TileRecord* tile_record = GetTileRecord(i, j, 1);
			if (tile_record && tile_record->finished_)
				finished_count++;
		}

		if (finished_count == cols)
		{
			TileRecord* tile_record = GetTileRecord(0, j, 1);

			if (tile_record)
			{
				resource_pool->Release(tile_record->poDataset_);
				tile_record->poDataset_ = nullptr;
			}
		}
	}
}

void SaveTileData(const std::string& path, const Aws::String& save_bucket_name
	, const Aws::String& obj_key, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id
	, size_t x, size_t y, size_t z, std::vector<unsigned char>& buffer)
{

#ifdef USE_FILE

	//test code
	if(0)
	{
		std::vector<unsigned char> buffer_dst;
		JpgCompress jpgCompress;
		jpgCompress.DoCompress(buffer.data(), 256, 256, buffer_dst);

		std::string path = "d:/test/s3test/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".jpg";

		FILE* pFile = nullptr;
		fopen_s(&pFile, path.c_str(), "wb+");
		fwrite(buffer_dst.data(), 1, buffer_dst.size(), pFile);
		fclose(pFile);
		pFile = nullptr;
	}

	std::string new_path = path + ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";

	FILE* pFile = nullptr;
	fopen_s(&pFile, new_path.c_str(), "wb+");
	if (!pFile)
	{
		std::cout << "file not open: " << new_path << std::endl;
	}

	fwrite(buffer.data(), 1, buffer.size(), pFile);
	fclose(pFile);
	pFile = nullptr;

#else

	std::string ext = ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";
	Aws::String obj_key_name = obj_key + Aws::String(ext.c_str(), ext.size());
	AWSS3PutObject(save_bucket_name, obj_key_name, region, aws_secret_access_key, aws_access_key_id, buffer);

#endif

}

bool LoadTileData(const std::string& path, const Aws::String& bucket_name
	, const Aws::String& obj_key, const Aws::String& region, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id, size_t x
	, size_t y, size_t z, std::vector<unsigned char>& buffer)
{
#ifdef USE_FILE
	std::string new_path = path + ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";

	std::ifstream inFile2(new_path, std::ios::binary | std::ios::in);
	inFile2.seekg(0, std::ios_base::end);
	int FileSize2 = inFile2.tellg();

	inFile2.seekg(0, std::ios::beg);

	//char* buffer2 = new char[FileSize2];
	buffer.resize(FileSize2);
	inFile2.read((char*)buffer.data(), FileSize2);
	inFile2.close();

	return true;

#else

	std::string ext = ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";
	Aws::String obj_key_name = obj_key + Aws::String(ext.c_str(), ext.size());
	return AWSS3GetObject(bucket_name, obj_key_name, region, aws_secret_access_key, aws_access_key_id, buffer);

#endif // USE_FILE

}

void ResampleNearest(unsigned char* buffer1, unsigned char* buffer2
	, unsigned char* buffer3, unsigned char* buffer4
	, unsigned char* buffer_dst, int pixel_space)
{
	for (size_t j = 0; j < 256; j++)
	{
		for (size_t i = 0; i < 256; i ++)
		{
			unsigned char* dst = buffer_dst + (j * 256 + i) * pixel_space;
			unsigned char* src = nullptr;

			if (i < 128 && j < 128)
			{
				src = buffer1 + (j * 256 + i) * 2 * pixel_space;
			}
			else if (i >= 128 && j < 128)
			{
				src = buffer2 + (j * 256 + i - 128) * 2 * pixel_space;
			}
			else if (i < 128 && j >= 128)
			{
				src = buffer3 + ((j - 128) * 256 + i) * 2 * pixel_space;
			}
			else if (i >= 125 && j >= 128)
			{
				src = buffer4 + ((j - 128) * 256 + i - 128) * 2 * pixel_space;
			}

			memcpy(dst, src, pixel_space);
		}
	}
}

void ResampleLinear(unsigned char* buffer1, unsigned char* buffer2
	, unsigned char* buffer3, unsigned char* buffer4, unsigned char* buffer_dst
	, int band_count, int type_bytes, GDALDataType data_type)
{
	int pixel_space = band_count * type_bytes;

	for (size_t j = 0; j < 256; j++)
	{
		for (size_t i = 0; i < 256; i++)
		{
			unsigned char* dst = buffer_dst + (j * 256 + i) * pixel_space;
			
			unsigned char* src1 = nullptr;
			unsigned char* src2 = nullptr;
			unsigned char* src3 = nullptr;
			unsigned char* src4 = nullptr;

			if (i < 128 && j < 128)
			{
				src1 = buffer1 + ((j * 2) * 256 + i * 2) * pixel_space;
				src2 = buffer1 + ((j * 2) * 256 + i * 2 + 1) * pixel_space;
				src3 = buffer1 + ((j * 2 + 1) * 256 + i * 2) * pixel_space;
				src4 = buffer1 + ((j * 2 + 1) * 256 + i * 2 + 1) * pixel_space;
			}
			else if (i >= 128 && j < 128)
			{
				src1 = buffer2 + ((j * 2) * 256 + (i - 128) * 2) * pixel_space;
				src2 = buffer2 + ((j * 2) * 256 + (i - 128) * 2 + 1) * pixel_space;
				src3 = buffer2 + ((j * 2 + 1) * 256 + (i - 128) * 2) * pixel_space;
				src4 = buffer2 + ((j * 2 + 1) * 256 + (i - 128) * 2 + 1) * pixel_space;
			}
			else if (i < 128 && j >= 128)
			{
				src1 = buffer3 + (((j - 128) * 2) * 256 + i * 2) * pixel_space;
				src2 = buffer3 + (((j - 128) * 2) * 256 + i * 2 + 1) * pixel_space;
				src3 = buffer3 + (((j - 128) * 2 + 1) * 256 + i * 2) * pixel_space;
				src4 = buffer3 + (((j - 128) * 2 + 1) * 256 + i * 2 + 1) * pixel_space;
			}
			else if (i >= 125 && j >= 128)
			{
				src1 = buffer4 + (((j - 128) * 2) * 256 + (i - 128) * 2) * pixel_space;
				src2 = buffer4 + (((j - 128) * 2) * 256 + (i - 128) * 2 + 1) * pixel_space;
				src3 = buffer4 + (((j - 128) * 2 + 1) * 256 + (i - 128) * 2) * pixel_space;
				src4 = buffer4 + (((j - 128) * 2 + 1) * 256 + (i - 128) * 2 + 1) * pixel_space;
			}

			for (int b = 0; b < band_count; b++)
			{
				unsigned char* src1_band = src1 + (size_t)b * type_bytes;
				unsigned char* src2_band = src2 + (size_t)b * type_bytes;
				unsigned char* src3_band = src3 + (size_t)b * type_bytes;
				unsigned char* src4_band = src4 + (size_t)b * type_bytes;

				unsigned char* dst_band = dst + (size_t)b * type_bytes;

				double value = 0.0;
				switch (data_type)
				{
				case GDT_Byte:
				{
					value = ((double)*src1_band + *src2_band + *src3_band + *src4_band) * 0.25;
					*dst_band = value;

					break;
				}
				case GDT_UInt16:
				{
					unsigned short* src1_band_t = (unsigned short*)src1_band;
					unsigned short* src2_band_t = (unsigned short*)src2_band;
					unsigned short* src3_band_t = (unsigned short*)src3_band;
					unsigned short* src4_band_t = (unsigned short*)src4_band;
					unsigned short* dst0_band_t = (unsigned short*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				case GDT_Int16:
				{
					short* src1_band_t = (short*)src1_band;
					short* src2_band_t = (short*)src2_band;
					short* src3_band_t = (short*)src3_band;
					short* src4_band_t = (short*)src4_band;
					short* dst0_band_t = (short*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				case GDT_UInt32:
				{
					unsigned int* src1_band_t = (unsigned int*)src1_band;
					unsigned int* src2_band_t = (unsigned int*)src2_band;
					unsigned int* src3_band_t = (unsigned int*)src3_band;
					unsigned int* src4_band_t = (unsigned int*)src4_band;
					unsigned int* dst0_band_t = (unsigned int*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				case GDT_Int32:
				{
					int* src1_band_t = (int*)src1_band;
					int* src2_band_t = (int*)src2_band;
					int* src3_band_t = (int*)src3_band;
					int* src4_band_t = (int*)src4_band;
					int* dst0_band_t = (int*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				case GDT_Float32:
				{
					float* src1_band_t = (float*)src1_band;
					float* src2_band_t = (float*)src2_band;
					float* src3_band_t = (float*)src3_band;
					float* src4_band_t = (float*)src4_band;
					float* dst0_band_t = (float*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				case GDT_Float64:
				{
					double* src1_band_t = (double*)src1_band;
					double* src2_band_t = (double*)src2_band;
					double* src3_band_t = (double*)src3_band;
					double* src4_band_t = (double*)src4_band;
					double* dst0_band_t = (double*)dst_band;

					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;

					break;
				}
				default:
					break;
				}
			}
		}
	}
}

void ProcessLoop(TaskRecord* task_record, int& status)
{
	long long start_sec = task_record->GetStartSec();
	int time_limit_sec = task_record->GetTimeLimitSec();

	while (!task_record->IsReady())
	{
		DoTask(task_record);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
		long long now_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();

		if ((now_sec - start_sec) > time_limit_sec)
		{
			status = code_not_finished;
			return;
		}
	}

	status = code_success;
}

void DoTask(TaskRecord* task_record)
{
	size_t tile_records_count = task_record->GetTileRecordsCount();
	int band_count = task_record->GetBandCount();
	GDALDataType data_type = task_record->GetDataType();
	std::string path = task_record->GetPath();
	Aws::String src_obj_key = task_record->GetSrcKey();
	Aws::String src_bucket = task_record->GetSrcBucket();
	Aws::String save_bucket = task_record->GetSaveBucket();

	Aws::String aws_secret_access_key = task_record->GetSecretAccessKey();
	Aws::String aws_access_key_id = task_record->GetAccessKeyID();

	std::vector<int> band_map;
	for (int i = 0; i < band_count; i ++)
	{
		band_map.push_back(i + 1);
	}

	int type_bytes = GDALGetDataTypeSizeBytes(data_type);
	size_t img_width = task_record->GetImgWidth();
	size_t img_height = task_record->GetImgHeight();

	int pixel_space = band_count * type_bytes;
	int line_space = 256 * pixel_space;
	int band_space = type_bytes;
	const Aws::String& region = task_record->GetRegion();

	for (size_t i = 0; i < tile_records_count; i++)
	{
		TileRecord* tile_record = task_record->GetTileRecord(i);

		if (tile_record->finished_ == false)
		{
			if (tile_record->processing_mutex_.try_lock())
			{
				if (tile_record->sub_tile_state_ == tile_record->sub_tile_state_expected)
				{
					std::vector<unsigned char> buffer;
					buffer.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

					//需要读原始影像
					if (tile_record->z_ == 1)
					{
						//dataset存到每行瓦片的第一个瓦片里
						//每行瓦片用同一个dataset，对于按行存储的数据，应该可以加速。
						TileRecord* tile_header = task_record->GetTileRecord(0, tile_record->row_, tile_record->z_);

						if (tile_header->poDataset_ == nullptr)
						{
							std::mutex& mutex_global = task_record->GetMutex();
							std::lock_guard<std::mutex> guard(mutex_global);

							//必须再判断一次。因为lock之前有可能被其他线程改变
							if (tile_header->poDataset_ == nullptr)
							{
								tile_header->poDataset_ = task_record->Acquire();
								//std::cout << "dataset: " << tile_header->poDataset_ << std::endl;
							}

							//如果没有获取到资源，则返回。
							if (tile_header->poDataset_ == nullptr)
							{
								tile_record->finished_ = false;
								tile_record->processing_mutex_.unlock();
								continue;
							}
						}

						//如果获取到了dataset，继续执行
						//测试header的dataset是否被占用
						if (tile_header->dataset_lock_flag_mutex_.try_lock())
						{
							int tile_size_img = 256;
							int z = tile_record->z_;

							while (z > 0)
							{
								z--;
								tile_size_img *= 2;
							}

							size_t left = tile_record->col_ * tile_size_img;
							size_t top = tile_record->row_ * tile_size_img;

							int src_width = std::min((size_t)tile_size_img, img_width - left);
							int src_height = std::min((size_t)tile_size_img, img_height - top);

							int buffer_size_x = 256;
							int buffer_size_y = 256;
							int line_space_cp = line_space;
							if (src_width != tile_size_img)
							{
								buffer_size_x = 256.0 * src_width / tile_size_img;
								line_space_cp = buffer_size_x * pixel_space;
							}

							if (src_height != tile_size_img)
							{
								buffer_size_y = 256.0 * src_height / tile_size_img;
							}

							GDALDataset* poDataset = tile_header->poDataset_;
							poDataset->RasterIO(GF_Read, left, top, src_width, src_height, buffer.data()
								, buffer_size_x, buffer_size_y, data_type, band_count, band_map.data(), pixel_space, line_space_cp, band_space);

							if (buffer_size_x != 256)
							{
								unsigned char* pbuffer = buffer.data();

								for (int m = buffer_size_y - 1; m >= 0; m--)
								{
									unsigned char* dst = pbuffer + m * 256 * pixel_space;
									unsigned char* src = pbuffer + m * buffer_size_x * pixel_space;

									memset(dst + buffer_size_x * pixel_space, 0, (256 - buffer_size_x) * pixel_space);

									for (int n = buffer_size_x - 1; n >= 0; n--)
									{
										memcpy(dst + n * pixel_space, src + n * pixel_space, pixel_space);
									}
								}
							}

							SaveTileData(path, save_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_, tile_record->row_, tile_record->z_, buffer);
							tile_record->finished_ = true;

							//标记上层瓦片
							TileRecord* parent = task_record->GetTileRecord(tile_record->col_ / 2, tile_record->row_ / 2, tile_record->z_ + 1);
							parent->sub_tile_state_++;

							tile_header->dataset_lock_flag_mutex_.unlock();
						}
					}
					else
					{
						std::vector<unsigned char> buffer1;
						buffer1.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

						std::vector<unsigned char> buffer2;
						buffer2.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

						std::vector<unsigned char> buffer3;
						buffer3.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

						std::vector<unsigned char> buffer4;
						buffer4.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

						unsigned char arrange = tile_record->sub_tile_arrange;
						
						if (arrange & 0x08)
						{
							LoadTileData(path, src_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_ * 2, tile_record->row_ * 2, tile_record->z_ - 1, buffer1);
						}

						if (arrange & 0x04)
						{
							LoadTileData(path, src_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_ * 2 + 1, tile_record->row_ * 2, tile_record->z_ - 1, buffer2);
						}

						if (arrange & 0x02)
						{
							LoadTileData(path, src_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_ * 2, tile_record->row_ * 2 + 1, tile_record->z_ - 1, buffer3);
						}

						if (arrange & 0x01)
						{
							LoadTileData(path, src_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_ * 2 + 1, tile_record->row_ * 2 + 1, tile_record->z_ - 1, buffer4);
						}

						//ResampleNearest(buffer1.data(), buffer2.data(), buffer3.data(), buffer4.data(), buffer.data(), pixel_space);
						ResampleLinear(buffer1.data(), buffer2.data(), buffer3.data(), buffer4.data(), buffer.data(), band_count, type_bytes, data_type);
						SaveTileData(path, save_bucket, src_obj_key, region, aws_secret_access_key, aws_access_key_id, tile_record->col_, tile_record->row_, tile_record->z_, buffer);

						tile_record->finished_ = true;

						//标记上层瓦片
						TileRecord* parent = task_record->GetTileRecord(tile_record->col_ / 2, tile_record->row_ / 2, tile_record->z_ + 1);
						if (parent)
						{
							parent->sub_tile_state_++;
						}

						break;
					}
				}

				tile_record->processing_mutex_.unlock();
			}
		}
	}

	task_record->ReleaseDataset();
}