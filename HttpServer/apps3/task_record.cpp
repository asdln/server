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
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#define USE_LAMBDA

char const TAG[] = "LAMBDA_ALLOC";


bool AWSS3GetObject(const Aws::String& fromBucket , const Aws::String& objectKey,
	Aws::S3::S3Client* s3_client, std::vector<unsigned char>& buffer)
{
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.SetBucket(fromBucket);
	object_request.SetKey(objectKey);

	Aws::S3::Model::GetObjectOutcome get_object_outcome =
		s3_client->GetObject(object_request);

	if (get_object_outcome.IsSuccess())
	{
		auto& retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
		size_t content_bytes = get_object_outcome.GetResultWithOwnership().GetContentLength();

		if (buffer.size() < content_bytes)
		{
			buffer.resize(content_bytes, 0);
		}

		retrieved_file.read((char*)buffer.data(), content_bytes);
		return true;
	}
	else
	{
		auto err = get_object_outcome.GetError();
		std::cout << "Error: GetObject: " <<
			err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

		std::cout << "bucket: " << fromBucket << "/t" << "key: " << objectKey << std::endl;

		return false;
	}
}

bool AWSS3PutObject(const Aws::String& bucketName, const Aws::String& objectName
	, Aws::S3::S3Client* s3_client, std::vector<unsigned char>& buffer)
{
	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(bucketName);
	request.SetKey(objectName);

	auto sbuff = Aws::New<Aws::Utils::Stream::PreallocatedStreamBuf>("whatever string",
		buffer.data(), buffer.size());

	auto sbody = Aws::MakeShared<Aws::IOStream>("whatever string", sbuff);
	request.SetBody(sbody);

	Aws::S3::Model::PutObjectOutcome outcome =
		s3_client->PutObject(request);

	if (outcome.IsSuccess()) {

		//std::cout << "Added object '" << objectName << "' to bucket '"
		//	<< bucketName << "'" << std::endl;;
		return true;
	}
	else
	{
		std::cout << "Error: PutObject: " <<
			outcome.GetError().GetMessage() << std::endl;

		std::cout << "bucket: " << bucketName << "/t" << "key: " << objectName << std::endl;

		return false;
	}
}

bool AWSS3DeleteObject(const Aws::String& objectKey,
	const Aws::String& fromBucket, const Aws::String& region
	, const Aws::String& aws_secret_access_key
	, const Aws::String& aws_access_key_id)
{
	Aws::Client::ClientConfiguration config;
	if (!region.empty())
	{
		config.region = region;

#ifdef USE_LAMBDA
		config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
#endif
	}

#ifdef USE_LAMBDA

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	Aws::S3::S3Client s3_client(credentialsProvider, config);

#else

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	Aws::S3::S3Client s3_client(cred, config);

#endif

	Aws::S3::Model::DeleteObjectRequest request;

	request.WithKey(objectKey)
		.WithBucket(fromBucket);

	Aws::S3::Model::DeleteObjectOutcome outcome =
		s3_client.DeleteObject(request);

	if (!outcome.IsSuccess())
	{
		auto err = outcome.GetError();
		std::cout << "Error: DeleteObject: " <<
			err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

		return false;
	}
	else
	{
		return true;
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

#ifdef USE_LAMBDA
		config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
#endif
	}

#ifdef USE_LAMBDA

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	Aws::S3::S3Client s3_client(credentialsProvider, config);

#else

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	Aws::S3::S3Client s3_client(cred, config);

#endif

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
			<< bucketName << "'";
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
	, const std::string& aws_access_key_id, int time_limit_sec, int force) : time_limit_sec_(time_limit_sec), force_(force)
{
	aws_region_ = Aws::String(region.c_str(), region.size());
	std::string save_bucket_name_temp = save_bucket_name;
	
	//去掉最前面的/和最后面的/	
	{
		int pos = save_bucket_name_temp.find('/');
		if (pos == 0)
		{
			save_bucket_name_temp = save_bucket_name_temp.substr(1, save_bucket_name_temp.size() - 1);
		}

		int rpos = save_bucket_name_temp.rfind('/');
		if (rpos == save_bucket_name_temp.size() - 1)
		{
			save_bucket_name_temp = save_bucket_name_temp.substr(0, save_bucket_name_temp.size() - 1);
		}
	}

	int pos = save_bucket_name_temp.find('/');
	if (pos == std::string::npos)
	{
		save_bucket_name_ = Aws::String(save_bucket_name_temp.c_str(), save_bucket_name_temp.size());
		save_key_name_ = "";
	}
	else
	{
		std::string save_bucket = save_bucket_name_temp.substr(0, pos);
		std::string save_key = save_bucket_name_temp.substr(pos + 1, save_bucket_name_temp.size() - pos - 1);

		save_bucket_name_ = Aws::String(save_bucket.c_str(), save_bucket.size());
		save_key_name_ = Aws::String(save_key.c_str(), save_key.size());
	}

	aws_access_key_id_ = Aws::String(aws_access_key_id.c_str(), aws_access_key_id.size());
	aws_secret_access_key_ = Aws::String(aws_secret_access_key.c_str(), aws_secret_access_key.size());

	std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
	start_sec_ = std::chrono::duration_cast<std::chrono::seconds>(d).count();
}

TaskRecord::~TaskRecord()
{
	std::string json = ToJson();

	size_t finished = 0;
	size_t unfinished = 0;
	GetTileStatusCount(finished, unfinished);
	std::cout << "end: finished tile: " << finished << "; unfinished tile : " << unfinished << std::endl;

#ifdef USE_FILE

	std::ofstream outFile(info_json_path_, std::ios::binary | std::ios::out);
	if (outFile.is_open())
	{
		std::cout << "save: " << info_json_path_ << std::endl;
		outFile.write(json.data(), json.size());
		outFile.close();
	}
	else
	{
		std::cout << "json error" << std::endl;
	}

#else

	std::vector<unsigned char> buffer;
	buffer.assign(json.begin(), json.end());

	Aws::Client::ClientConfiguration config;

	if (!aws_region_.empty())
	{
		config.region = aws_region_;

#ifdef USE_LAMBDA
		config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
#endif
	}

#ifdef USE_LAMBDA

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	Aws::S3::S3Client s3_client(credentialsProvider, config);

#else

	Aws::Auth::AWSCredentials cred(aws_access_key_id_, aws_secret_access_key_);
	Aws::S3::S3Client s3_client(cred, config);

#endif

	if (AWSS3PutObject(save_bucket_name_, save_key_name_ + ".pyra/info.json"
		, &s3_client, buffer))
	{
		std::cout << "json saved:  " << save_key_name_ + ".pyra/info.json" << std::endl;
	}

#endif

	for (auto record : tile_records_)
	{
		delete record;
	}

	std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
	long long end_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();

	std::cout << "process finished, total time: " << end_sec - start_sec_ << " seconds" << std::endl;

	std::cout << "compute histogram ..." << std::endl;
}

bool TaskRecord::Open(const std::string& path, int dataset_count)
{
	path_ = path;
	format_ = "bip";

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

	size_t rpos = temp_str.rfind('/');
	if (rpos != std::string::npos)
	{
		std::string file_name = temp_str.substr(rpos + 1, temp_str.size() - rpos - 1);
		save_key_name_ += '/';
		save_key_name_ += Aws::String(file_name.c_str(), file_name.size());
	}

#endif // USE_FILE

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
	nodata_value_ = poDataset->GetRasterBand(1)->GetNoDataValue(&have_nodata_value_);

	GDALClose(poDataset);
	poDataset = nullptr;

// 	resource_pool = new ResourcePool;
// 	resource_pool->SetPath(path);
// 	resource_pool->SetDatasetPoolMaxCount(dataset_count);
//	resource_pool->Init();

	int size = 256;
	size_t index = 0;
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

		dims_.push_back(std::make_pair(tile_col, tile_row));

		for (size_t j = 0; j < tile_row; j++)
		{
			for (size_t i = 0; i < tile_col; i++)
			{
				TileRecord* tile_record = new TileRecord(i, j, index);
				tile_records_.push_back(tile_record);

				//如果是1级，则数据已经准备好了(直接读原始数据)
				if (index == 1)
				{
					//默认全部就绪。边角的地方，在读取时做特殊处理。
					tile_record->sub_tile_state_expected_ = 4;
					tile_record->sub_tile_arrange_ = 0x0f;
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
						tile_record->sub_tile_state_expected_ = 4;
						tile_record->sub_tile_arrange_ = 0x0f; //1111
					}
					else if (ii == 1 && jj == 1)
					{
						tile_record->sub_tile_state_expected_ = 1;
						tile_record->sub_tile_arrange_ = 0x08; // 1000
					}
					else if (ii == 0 && jj == 1)
					{
						tile_record->sub_tile_state_expected_ = 2;
						tile_record->sub_tile_arrange_ = 0x0c; //1100
					}
					else if (ii == 1 && jj == 0)
					{
						tile_record->sub_tile_state_expected_ = 2;
						tile_record->sub_tile_arrange_ = 0x0a; //1010
					}
				}
			}
		}

		if (tile_col == 1 && tile_row == 1)
			break;
	}

	info_json_path_ = dir + "/info.json";

#ifdef USE_FILE

	std::ifstream inFile2(info_json_path_, std::ios::binary | std::ios::in);
	if (force_ == 0 && inFile2.is_open())
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
		FromJson(string_json);
	}

#else

	Aws::Client::ClientConfiguration config;

	if (!aws_region_.empty())
	{
		config.region = aws_region_;

#ifdef USE_LAMBDA
		config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
#endif
	}

#ifdef USE_LAMBDA

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	Aws::S3::S3Client s3_client(credentialsProvider, config);

#else

	Aws::Auth::AWSCredentials cred(aws_access_key_id_, aws_secret_access_key_);
	Aws::S3::S3Client s3_client(cred, config);

#endif

	std::vector<unsigned char> buffer;
	if (force_ == 0 && AWSS3GetObject(save_bucket_name_, save_key_name_ + ".pyra/info.json"
		, &s3_client, buffer))
	{
		std::string string_json(buffer.begin(), buffer.end());
		FromJson(string_json);
	}

#endif // USE_FILE

	size_t finished = 0;
	size_t unfinished = 0;
	GetTileStatusCount(finished, unfinished);
	std::cout << "begin: finished tile: " << finished << "; unfinished tile : " << unfinished << std::endl;
	
	return true;
}

std::string TaskRecord::ToJson()
{
	neb::CJsonObject oJson;
	oJson.Add("format", format_);
	oJson.Add("count", tile_records_.size());
	oJson.AddEmptySubArray("tile_finished");
	oJson.AddEmptySubArray("sub_tile_state");

	for (auto tile_record : tile_records_)
	{
		oJson["tile_finished"].Add((int)(tile_record->finished_));
		oJson["sub_tile_state"].Add(tile_record->sub_tile_state_);
	}

	return oJson.ToString();
}

bool TaskRecord::FromJson(const std::string& json)
{
	if (json.empty())
	{
		std::cout << "json is empty" << std::endl;
		return false;
	}

	neb::CJsonObject oJson(json);

	int count;

	if (!oJson.Get("count", count))
	{
		std::cout << "json not find cout" << std::endl;
		return false;
	}

	if (!oJson.Get("format", format_))
	{
		std::cout << "json not find format" << std::endl;
		return false;
	}

	if (count == tile_records_.size())
	{
		std::cout << info_json_path_ << " loaded" << std::endl;

		for (int i = 0; i < count; i++)
		{
			TileRecord* tile_record = tile_records_[i];

			int tile_finished = 0;
			oJson["tile_finished"].Get(i, tile_finished);
			tile_record->finished_ = tile_finished;

			int sub_tile_state;
			oJson["sub_tile_state"].Get(i, sub_tile_state);
			tile_record->sub_tile_state_ = sub_tile_state;
		}
	}
	else
	{
		std::cout << info_json_path_ << "not loaded, because count is not equal" << std::endl;
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
	for (auto tile_record : tile_records_)
	{
		if (tile_record->finished_ == false)
			return false;
	}

	return true;
}

void TaskRecord::GetTileStatusCount(size_t& finished, size_t& unfinished)
{
	finished = 0;
	unfinished = 0;

	for (auto tile_record : tile_records_)
	{
		if (tile_record->finished_ == false)
		{
			unfinished++;
		}
		else
		{
			finished++;
		}
	}
}

// bool TaskRecord::NeedReleaseDataset()
// {
// 	return resource_pool->NeedReleaseDataset();
// }

// void TaskRecord::ReleaseDataset()
// {
// 	if (!NeedReleaseDataset())
// 		return;
// 
// 	std::lock_guard<std::mutex> guard(mutex_global_);
// 
// 	size_t cols = dims_[0].first;
// 	size_t rows = dims_[0].second;
// 
// 	for (size_t j = 0; j < rows; j++)
// 	{
// 		size_t finished_count = 0;
// 		for (size_t i = 0; i < cols; i++)
// 		{
// 			TileRecord* tile_record = GetTileRecord(i, j, 1);
// 			if (tile_record && tile_record->finished_)
// 				finished_count++;
// 		}
// 
// 		if (finished_count == cols)
// 		{
// 			TileRecord* tile_record = GetTileRecord(0, j, 1);
// 
// 			if (tile_record)
// 			{
// 				resource_pool->Release(tile_record->poDataset_);
// 				tile_record->poDataset_ = nullptr;
// 			}
// 		}
// 	}
// }

void TaskRecord::GetDims(int z, int& cols, int& rows)
{
	cols = dims_[z - 1].first;
	rows = dims_[z - 1].second;
}

void SaveTileData(const std::string& path, const Aws::String& save_bucket_name
	, const Aws::String& obj_key, Aws::S3::S3Client* s3_client
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
	AWSS3PutObject(save_bucket_name, obj_key_name, s3_client, buffer);

#endif

}

bool LoadTileData(const std::string& path, const Aws::String& bucket_name
	, const Aws::String& obj_key, Aws::S3::S3Client* s3_client, size_t x
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
	return AWSS3GetObject(bucket_name, obj_key_name, s3_client, buffer);

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

template <typename T>
void fourInOne(unsigned char* buffer1, unsigned char* buffer2
	, unsigned char* buffer3, unsigned char* buffer4, unsigned char* buffer_dst
	, int band_count, int type_bytes, GDALDataType data_type, int have_nodata, double nodata_value)
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

				T* src1_band_t = (T*)src1_band;
				T* src2_band_t = (T*)src2_band;
				T* src3_band_t = (T*)src3_band;
				T* src4_band_t = (T*)src4_band;
				T* dst0_band_t = (T*)dst_band;

				if (have_nodata)
				{
					unsigned char count = 4;
					unsigned char tag1 = 1;
					unsigned char tag2 = 1;
					unsigned char tag3 = 1;
					unsigned char tag4 = 1;

					if (*src1_band_t == nodata_value)
					{
						tag1 = 0;
						count--;
					}

					if (*src2_band_t == nodata_value)
					{
						tag2 = 0;
						count--;
					}

					if (*src3_band_t == nodata_value)
					{
						tag3 = 0;
						count--;
					}

					if (*src4_band_t == nodata_value)
					{
						tag4 = 0;
						count--;
					}

					if (count == 0)
					{
						*dst0_band_t = nodata_value;
					}
					else
					{
						value = ((double)*src1_band_t * tag1 + *src2_band_t * tag2 + *src3_band_t * tag3 + *src4_band_t * tag4) * (1.0 / count);
						*dst0_band_t = value;
					}
				}
				else
				{
					value = ((double)*src1_band_t + *src2_band_t + *src3_band_t + *src4_band_t) * 0.25;
					*dst0_band_t = value;
				}
			}
		}
	}
}

void ResampleFourInOne(unsigned char* buffer1, unsigned char* buffer2
	, unsigned char* buffer3, unsigned char* buffer4, unsigned char* buffer_dst
	, int band_count, int type_bytes, GDALDataType data_type, int have_nodata, double nodata_value)
{
	switch (data_type)
	{
	case GDT_Byte:
	{
		fourInOne<unsigned char>(buffer1, buffer2 , buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_UInt16:
	{
		fourInOne<unsigned short>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_Int16:
	{
		fourInOne<short>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_UInt32:
	{
		fourInOne<unsigned int>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_Int32:
	{
		fourInOne<int>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_Float32:
	{
		fourInOne<float>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	case GDT_Float64:
	{
		fourInOne<double>(buffer1, buffer2, buffer3, buffer4, buffer_dst
			, band_count, type_bytes, data_type, have_nodata, nodata_value);

		break;
	}
	default:
		break;
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

void ProcessSingleTileFromImage(TileRecord* tile_record, TaskRecord* task_record, Aws::S3::S3Client* s3_client, GDALDataset* poDataset)
{
	int band_count = task_record->GetBandCount();
	GDALDataType data_type = task_record->GetDataType();
	std::string path = task_record->GetPath();
	Aws::String save_obj_key = task_record->GetSaveKey();
	Aws::String save_bucket = task_record->GetSaveBucket();

	std::vector<int> band_map;
	for (int i = 0; i < band_count; i++)
	{
		band_map.push_back(i + 1);
	}

	int type_bytes = GDALGetDataTypeSizeBytes(data_type);
	size_t img_width = task_record->GetImgWidth();
	size_t img_height = task_record->GetImgHeight();

	int pixel_space = band_count * type_bytes;
	int line_space = 256 * pixel_space;
	int band_space = type_bytes;

	if (tile_record->processing_mutex_.try_lock())
	{
		std::vector<unsigned char> buffer;
		buffer.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

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

		CPLErr err = poDataset->RasterIO(GF_Read, left, top, src_width, src_height, buffer.data()
			, buffer_size_x, buffer_size_y, data_type, band_count, band_map.data(), pixel_space, line_space_cp, band_space);

		if (err != CE_None)
		{
			std::cout << CPLGetLastErrorMsg() << std::endl;
		}

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

		SaveTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_, tile_record->row_, tile_record->z_, buffer);
		tile_record->finished_ = true;

		//标记上层瓦片
		TileRecord* parent = task_record->GetTileRecord(tile_record->col_ / 2, tile_record->row_ / 2, tile_record->z_ + 1);
		parent->sub_tile_state_++;

		tile_record->processing_mutex_.unlock();
	}
}

void ProcessSingleTileFromTile(TileRecord* tile_record, TaskRecord* task_record, Aws::S3::S3Client* s3_client)
{
	size_t tile_records_count = task_record->GetTileRecordsCount();
	int band_count = task_record->GetBandCount();
	GDALDataType data_type = task_record->GetDataType();
	std::string path = task_record->GetPath();
	Aws::String save_obj_key = task_record->GetSaveKey();
	Aws::String save_bucket = task_record->GetSaveBucket();

	int have_nodata = 0;
	double nodata_value = task_record->GetNoDataValue(have_nodata);

	std::vector<int> band_map;
	for (int i = 0; i < band_count; i++)
	{
		band_map.push_back(i + 1);
	}

	int type_bytes = GDALGetDataTypeSizeBytes(data_type);
	size_t img_width = task_record->GetImgWidth();
	size_t img_height = task_record->GetImgHeight();

	int pixel_space = band_count * type_bytes;
	int line_space = 256 * pixel_space;
	int band_space = type_bytes;

	std::vector<unsigned char> buffer;
	buffer.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

	std::vector<unsigned char> buffer1;
	buffer1.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

	std::vector<unsigned char> buffer2;
	buffer2.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

	std::vector<unsigned char> buffer3;
	buffer3.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

	std::vector<unsigned char> buffer4;
	buffer4.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

	unsigned char arrange = tile_record->sub_tile_arrange_;

	if (arrange & 0x08)
	{
		LoadTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_ * 2, tile_record->row_ * 2, tile_record->z_ - 1, buffer1);
	}

	if (arrange & 0x04)
	{
		LoadTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_ * 2 + 1, tile_record->row_ * 2, tile_record->z_ - 1, buffer2);
	}

	if (arrange & 0x02)
	{
		LoadTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_ * 2, tile_record->row_ * 2 + 1, tile_record->z_ - 1, buffer3);
	}

	if (arrange & 0x01)
	{
		LoadTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_ * 2 + 1, tile_record->row_ * 2 + 1, tile_record->z_ - 1, buffer4);
	}

	//ResampleNearest(buffer1.data(), buffer2.data(), buffer3.data(), buffer4.data(), buffer.data(), pixel_space);
	//ResampleLinear(buffer1.data(), buffer2.data(), buffer3.data(), buffer4.data(), buffer.data(), band_count, type_bytes, data_type);
	ResampleFourInOne(buffer1.data(), buffer2.data(), buffer3.data(), buffer4.data(), buffer.data(), band_count, type_bytes, data_type, have_nodata, nodata_value);
	SaveTileData(path, save_bucket, save_obj_key, s3_client, tile_record->col_, tile_record->row_, tile_record->z_, buffer);

	tile_record->finished_ = true;

	//标记上层瓦片
	TileRecord* parent = task_record->GetTileRecord(tile_record->col_ / 2, tile_record->row_ / 2, tile_record->z_ + 1);
	if (parent)
	{
		parent->sub_tile_state_++;
	}
}

void ProcessStripTileFromImage(TileRecord* tile_record, TaskRecord* task_record, Aws::S3::S3Client* s3_client, GDALDataset* poDataset)
{
	size_t tile_records_count = task_record->GetTileRecordsCount();
	int band_count = task_record->GetBandCount();
	GDALDataType data_type = task_record->GetDataType();
	std::string path = task_record->GetPath();
	Aws::String save_obj_key = task_record->GetSaveKey();
	Aws::String save_bucket = task_record->GetSaveBucket();

	std::vector<int> band_map;
	for (int i = 0; i < band_count; i++)
	{
		band_map.push_back(i + 1);
	}

	int type_bytes = GDALGetDataTypeSizeBytes(data_type);
	size_t img_width = task_record->GetImgWidth();
	size_t img_height = task_record->GetImgHeight();

	int pixel_space = band_count * type_bytes;
	int line_space = 256 * pixel_space;
	int band_space = type_bytes;

	int cols, rows;
	task_record->GetDims(tile_record->z_, cols, rows);

	constexpr int tile_size_img = 256 * 2;
	size_t left = 0;
	size_t top = tile_record->row_ * tile_size_img;

	int src_width = img_width;
	int src_height = std::min((size_t)tile_size_img, img_height - top);

	int buffer_size_x = src_width * 0.5;
	int buffer_size_y = src_height * 0.5;

	std::vector<unsigned char> buffer;
	buffer.resize((size_t)buffer_size_x * buffer_size_y * band_count * type_bytes, 0);

	int line_space_cp = buffer_size_x * pixel_space;
	CPLErr err = poDataset->RasterIO(GF_Read, left, top, src_width, src_height, buffer.data()
		, buffer_size_x, buffer_size_y, data_type, band_count, band_map.data(), pixel_space, line_space_cp, band_space);

	if (err != CE_None)
	{
		std::cout << CPLGetLastErrorMsg() << std::endl;
	}

	for (int m = 0; m < cols; m++)
	{
		TileRecord* tile_temp = task_record->GetTileRecord(m, tile_record->row_, tile_record->z_);
		if (tile_temp->finished_ == false)
		{
			size_t sub_left = m * 256;
			size_t sub_top = 0;

			int sub_buffer_width = std::min((size_t)256, buffer_size_x - sub_left);
			int sub_buffer_height = std::min((size_t)256, buffer_size_y - sub_top);

			std::vector<unsigned char> sub_buffer;
			sub_buffer.resize(256 * 256 * (size_t)band_count * type_bytes, 0);

			for (int n = 0; n < sub_buffer_height; n++)
			{
				unsigned char* dst = sub_buffer.data() + n * 256 * pixel_space;
				unsigned char* src = buffer.data() + (n * buffer_size_x + sub_left) * pixel_space;

				memcpy(dst, src, sub_buffer_width * pixel_space);
			}

			SaveTileData(path, save_bucket, save_obj_key, s3_client, tile_temp->col_, tile_temp->row_, tile_temp->z_, sub_buffer);
			tile_temp->finished_ = true;

			//标记上层瓦片
			TileRecord* parent = task_record->GetTileRecord(tile_temp->col_ / 2, tile_temp->row_ / 2, tile_temp->z_ + 1);
			parent->sub_tile_state_++;
		}
	}
}

void DoTask(TaskRecord* task_record, Aws::S3::S3Client* s3_client)
{
	size_t tile_records_count = task_record->GetTileRecordsCount();
	int band_count = task_record->GetBandCount();
	GDALDataType data_type = task_record->GetDataType();
	std::string path = task_record->GetPath();
	Aws::String save_obj_key = task_record->GetSaveKey();
	Aws::String save_bucket = task_record->GetSaveBucket();

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
	

	for (size_t i = 0; i < tile_records_count; i++)
	{
		TileRecord* tile_record = task_record->GetTileRecord(i);

		//需要读原始影像
		if (tile_record->z_ == 1 && tile_record->finished_ == false)
		{
			TileRecord* tile_header = task_record->GetTileRecord(0, tile_record->row_, tile_record->z_);
			if (tile_header->processing_mutex_.try_lock())
			{
// 				if (tile_header->poDataset_ == nullptr)
// 				{
// 					std::mutex& mutex_global = task_record->GetMutex();
// 					std::lock_guard<std::mutex> guard(mutex_global);
// 
// 					//必须再判断一次。因为lock之前有可能被其他线程改变
// 					if (tile_header->poDataset_ == nullptr)
// 					{
// 						tile_header->poDataset_ = task_record->Acquire();
// 						//std::cout << "dataset: " << tile_header->poDataset_ << std::endl;
// 					}
// 
// 					//如果没有获取到资源，则返回。
// 					if (tile_header->poDataset_ == nullptr)
// 					{
// 						//tile_record->finished_ = false;
// 						tile_header->processing_mutex_.unlock();
// 
// 						std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 						continue;
// 					}
// 				}

				//if (tile_header->dataset_lock_flag_mutex_.try_lock())
				{
					//如果条带中有大部分没有加载，则一次性读取整个条带
					int cols, rows;
					task_record->GetDims(tile_record->z_, cols, rows);

					int finished_count = 0;

					for (int m = 0; m < cols; m++)
					{
						TileRecord* tile_temp = task_record->GetTileRecord(m, tile_record->row_, tile_record->z_);
						if (tile_temp->finished_ == 1)
							finished_count++;
					}

					GDALDataset* poDataset = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);

					//如果有一半以上没加载，则整条带加载
					if (finished_count < cols * 0.5)
					{
						ProcessStripTileFromImage(tile_record, task_record, s3_client, poDataset);
					}
					else //单块处理
					{
						ProcessSingleTileFromImage(tile_record, task_record, s3_client, poDataset);
					}

					GDALClose(poDataset);

					//tile_header->dataset_lock_flag_mutex_.unlock();
				}

				tile_header->processing_mutex_.unlock();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			
			break;
		}
		else if (tile_record->finished_ == false)
		{
			if (tile_record->processing_mutex_.try_lock())
			{
				if (tile_record->sub_tile_state_ == tile_record->sub_tile_state_expected_)
				{
					ProcessSingleTileFromTile(tile_record, task_record, s3_client);
					tile_record->processing_mutex_.unlock();
					break;
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}

				tile_record->processing_mutex_.unlock();
			}
		}
	}

	//task_record->ReleaseDataset();
}

void ProcessLoop(TaskRecord* task_record, int& status)
{
	Aws::String aws_secret_access_key = task_record->GetSecretAccessKey();
	Aws::String aws_access_key_id = task_record->GetAccessKeyID();
	const Aws::String& region = task_record->GetRegion();

	Aws::S3::S3Client* s3_client = nullptr;

#ifndef USE_FILE

	Aws::Client::ClientConfiguration config;
	if (!region.empty())
	{
		config.region = region;

#ifdef USE_LAMBDA
		config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
#endif
	}

#ifdef USE_LAMBDA

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	s3_client = new Aws::S3::S3Client(credentialsProvider, config);
	//Aws::S3::S3Client s3_client(credentialsProvider, config);

#else

	Aws::Auth::AWSCredentials cred(aws_access_key_id, aws_secret_access_key);
	s3_client = new Aws::S3::S3Client(cred, config);
	//Aws::S3::S3Client s3_client(cred, config);

#endif

#endif

	long long start_sec = task_record->GetStartSec();
	int time_limit_sec = task_record->GetTimeLimitSec();

	while (!task_record->IsReady())
	{
		try
		{
			DoTask(task_record, s3_client);
		}
		catch (...)
		{
			std::cout << "ln_debug error occured" << std::endl;
		}
		
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
		long long now_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();

		if ((now_sec - start_sec) > time_limit_sec)
		{
			status = code_not_finished;

			if (s3_client != nullptr)
			{
				delete s3_client;
			}

			return;
		}
	}

	status = code_success;

	if (s3_client != nullptr)
	{
		delete s3_client;
	}
}