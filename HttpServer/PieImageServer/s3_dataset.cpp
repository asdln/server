#include "s3_dataset.h"
#include <iostream>
#include <fstream>

#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/auth/AWSCredentials.h>
#include "gdal_priv.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

Aws::String g_aws_region;

Aws::String g_aws_secret_access_key;

Aws::String g_aws_access_key_id;

bool AWSS3GetObject(Aws::S3::S3Client* s3_client, const Aws::String& fromBucket, const Aws::String& objectKey
	, char** buffer, MemoryPool* pool)
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

		//*buffer = new char[content_bytes];
		*buffer = (char*)pool->malloc(content_bytes);

		retrieved_file.read(*buffer, content_bytes);
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

bool LoadTileData(Aws::S3::S3Client* s3_client, const std::string& path
	, const Aws::String& src_bucket_name, const Aws::String& src_key_name
	, size_t x, size_t y, size_t z, char** buffer, MemoryPool* pool)
{
	std::string str_prefix = ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";
	std::string new_path = path + str_prefix;

#ifdef USE_FILE

	std::ifstream inFile2(new_path, std::ios::binary | std::ios::in);
	if (!inFile2.is_open())
		return false;

	inFile2.seekg(0, std::ios_base::end);
	int FileSize2 = inFile2.tellg();

	inFile2.seekg(0, std::ios::beg);

	*buffer = new char[FileSize2];
	inFile2.read(*buffer, FileSize2);
	inFile2.close();

	return true;

#else

	//std::string temp_str = path;
	//if (temp_str.rfind("/vsis3/", 0) != 0)
	//{
	//	std::cout << "error: path not begin with \"/vsis3/\"" << std::endl;
	//	return false;
	//}

	//temp_str = std::string(temp_str.c_str() + 7, temp_str.size() - 7);
	//int pos = temp_str.find("/");

	//Aws::String src_bucket_name = Aws::String(temp_str.c_str(), pos);
	//Aws::String src_key_name = Aws::String(temp_str.c_str() + pos + 1, temp_str.size() - pos - 1);
	Aws::String key_name = src_key_name;
	key_name += str_prefix;

	return AWSS3GetObject(s3_client, src_bucket_name, key_name, buffer, pool);

#endif // USE_FILE

}

bool S3Dataset::Open(const std::string& path)
{
	//清理一下。否则s3新上传的数据打不开
	VSICurlClearCache();

	if (!TiffDataset::Open(path))
		return false;

	size_t img_width = TiffDataset::GetRasterXSize();
	size_t img_height = TiffDataset::GetRasterYSize();

	int size = 256;

	while (true)
	{
		size *= 2;
		size_t tile_col = img_width % size == 0 ? img_width / size : img_width / size + 1;
		size_t tile_row = img_height % size == 0 ? img_height / size : img_height / size + 1;

		if (tile_col == 0)
			tile_col = 1;

		if (tile_row == 0)
			tile_row = 1;

		dims_.push_back(std::make_pair(tile_col, tile_row));
		if (tile_col == 1 && tile_row == 1)
			break;
	}

	type_bytes_ = GetDataTypeBytes(GetDataType());

#ifndef USE_FILE


	try
	{
		Aws::Client::ClientConfiguration config;

		if (g_aws_access_key_id.empty())
		{
			std::cout << "error: getenv(\"AWS_ACCESS_KEY_ID\")" << g_aws_access_key_id << std::endl;
		}

		config.region = g_aws_region;

		Aws::Auth::AWSCredentials cred(g_aws_access_key_id, g_aws_secret_access_key);
		s3_client_ = new Aws::S3::S3Client(cred, config);
	}
	catch (...)
	{
		std::cout << "s3 client failed" << std::endl;
		return false;
	}

#endif

	//std::string temp_str = path;
	//if (temp_str.rfind("/vsis3/", 0) != 0)
	//{
	//	std::cout << "error: path not begin with \"/vsis3/\"" << std::endl;
	//	return false;
	//}

	//temp_str = std::string(temp_str.c_str() + 7, temp_str.size() - 7);
	//int pos = temp_str.find("/");

	//s3_bucket_name_ = Aws::String(temp_str.c_str(), pos);
	//s3_key_name_ = Aws::String(temp_str.c_str() + pos + 1, temp_str.size() - pos - 1);

	return true;
}

void S3Dataset::SetS3CacheKey(const std::string& s3cachekey)
{
	if (s3cachekey.empty())
	{
		switch_ = false;
		s3_bucket_name_.clear();
		s3_key_name_.clear();
		return;
	}

	switch_ = true;
	std::string s3cachekey_temp = s3cachekey;

	//去掉最前面的/和最后面的/	
	{
		int pos = s3cachekey.find('/');
		if (pos == 0)
		{
			s3cachekey_temp = s3cachekey.substr(1, s3cachekey.size() - 1);
		}

		int rpos = s3cachekey.rfind('/');
		if (rpos == s3cachekey.size() - 1)
		{
			s3cachekey_temp = s3cachekey.substr(0, s3cachekey.size() - 1);
		}
	}

	int pos = s3cachekey_temp.find('/');
	if (pos == std::string::npos)
	{
		s3_bucket_name_ = Aws::String(s3cachekey_temp.c_str(), s3cachekey_temp.size());
		s3_key_name_ = "";
	}
	else
	{
		std::string save_bucket = s3cachekey_temp.substr(0, pos);
		std::string save_key = s3cachekey_temp.substr(pos + 1, s3cachekey_temp.size() - pos - 1);

		s3_bucket_name_ = Aws::String(save_bucket.c_str(), save_bucket.size());
		s3_key_name_ = Aws::String(save_key.c_str(), save_key.size());
	}

	size_t rpos = file_path_.rfind('/');
	if (rpos != std::string::npos)
	{
		std::string file_name = file_path_.substr(rpos + 1, file_path_.size() - rpos - 1);
		s3_key_name_ += '/';
		s3_key_name_ += Aws::String(file_name.c_str(), file_name.size());
	}

}

bool S3Dataset::Read(int nx, int ny, int width, int height,void* pData, int bufferWidth
	, int bufferHeight, DataType dataType, int nBandCount, int* pBandMap
	, long long pixSpace , long long lineSapce, long long bandSpace, void* psExtraArg)
{
	int level = 1;
	double scale = std::min((double)width / bufferWidth, (double)height / bufferHeight);

	//如果没有金字塔，隔一段时间再判断一下
	if (!s3_bucket_name_.empty() && switch_ == false)
	{
		boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::universal_time();
		boost::posix_time::millisec_posix_time_system_config::time_duration_type time_elapse = time_now - time_for_change_;

		//20秒
		if (time_elapse.total_seconds() >= 20)
		{
			switch_ = true;
			LOG(ERROR) << "change s3 pyramid check status to true";
		}
	}

	if (scale <= 1.0 || s3_bucket_name_.empty() || switch_ == false)
	{
		//std::cout << "<<tiffdataset_read";
		return TiffDataset::Read(nx, ny, width, height, pData, bufferWidth, bufferHeight
			, dataType, nBandCount, pBandMap, pixSpace, lineSapce, bandSpace, psExtraArg);
	}

	double scale0 = 2.0;
	int tile_size_src = 512;

	int pixel_bytes_bin = type_bytes_ * GetBandCount();
	int max_level = dims_.size();
	
	while (true)
	{
		double min = scale0;
		double max = scale0 * 2.0;
		
		if (scale >= min && scale <= max)
		{
			break;
		}

		if (level >= max_level)
			break;

		tile_size_src *= 2;
		scale0 *= 2.0;
		level++;
	}

	int src_left = nx;
	int src_top = ny;
	int src_right = nx + width - 1;
	int src_bottom = ny + height - 1;

	int index_left = src_left / tile_size_src;
	int index_top = src_top / tile_size_src;
	int index_right = src_right / tile_size_src;
	int index_bottom = src_bottom / tile_size_src;

	std::vector<unsigned char * > buffers;
	for (int n = index_top; n <= index_bottom; n++)
	{
		for (int m = index_left; m <= index_right; m++)
		{
			//std::vector<unsigned char> buffer;
			unsigned char* buffer = nullptr;

			//如果不成功，则读原始数据
			bool res = LoadTileData(s3_client_, file_path_, s3_bucket_name_, s3_key_name_, m, n, level, (char**)&buffer, mem_pool_);
			if (false == res)
			{
				for (auto p : buffers)
				{
					mem_pool_->free(p);
				}

				LOG(ERROR) << "error: can not load s3 tile data, use origin tiff instead";
				s3_failed_count_++;

				//连续4次读取不成功，则认为没有s3金字塔
				if (s3_failed_count_ >= 4)
				{
					s3_failed_count_ = 0;
					switch_ = false;
					time_for_change_ = boost::posix_time::microsec_clock::universal_time();
					LOG(ERROR) << "change s3 pyramid check status to false";
				}

				return TiffDataset::Read(nx, ny, width, height, pData, bufferWidth, bufferHeight
					, dataType, nBandCount, pBandMap, pixSpace, lineSapce, bandSpace, psExtraArg);
			}
			else
			{
				//有一次读取成功了，则重新计数
				s3_failed_count_ = 0;
			}

			buffers.emplace_back(buffer);
		}
	}

	int block_count_x = index_right - index_left + 1;
	int block_count_y = index_bottom - index_top + 1;

	int nTypeSize = GetDataTypeBytes(dataType);
	int level_left = index_left * 256;
	int level_top = index_top * 256;

	double res_x = (double)width / bufferWidth;
	double res_y = (double)height / bufferHeight;

	double mul = 1.0 / scale0;
	for (int j = 0; j < bufferHeight; j ++)
	{
		double y = (ny + 0.5 + j * res_y) * mul;
		int int_y = (int)y;
		double v = y - int_y;
		int_y = int_y - level_top;

		for (int i = 0; i < bufferWidth; i++)
		{
			double x = (nx + 0.5 + i * res_x) * mul;
			int int_x = (int)x;
			double u = x - int_x;
			int_x = int_x - level_left;

			int sample_x[4];
			int sample_y[4];

			sample_x[0] = int_x;
			sample_y[0] = int_y;

			sample_x[1] = int_x;
			sample_y[1] = int_y + 1;

			sample_x[2] = int_x + 1;
			sample_y[2] = int_y;

			sample_x[3] = int_x + 1;
			sample_y[3] = int_y + 1;

			unsigned char* src_data[4];

			for (int w = 0; w < 4; w++)
			{
				int block_x_index = sample_x[w] / 256;
				int block_y_index = sample_y[w] / 256;

				sample_x[w] = sample_x[w] % 256;
				sample_y[w] = sample_y[w] % 256;

				int block_index = block_y_index * block_count_x + block_x_index;
				unsigned char* src = buffers[block_index];

				src = src + (sample_y[w] * 256 + sample_x[w]) * pixel_bytes_bin;
				src_data[w] = src;
			}

			unsigned char* value1 = src_data[0];
			unsigned char* value2 = src_data[1];
			unsigned char* value3 = src_data[2];
			unsigned char* value4 = src_data[3];

			unsigned char* pDes = (unsigned char*)pData + (bufferWidth * j + i) * type_bytes_ * nBandCount;
			double p1 = (1.0 - u) * (1.0 - v);
			double p2 = (1.0 - u) * v;
			double p3 = u * (1.0 - v);
			double p4 = u * v;

			for (int band = 0; band < nBandCount; band++)
			{
				int bandIndex = pBandMap[band] - 1;
				int offset = bandIndex * nTypeSize;
				switch (dataType)
				{
				case DT_Byte:
				{
					unsigned char value = p1 * *(value1 + offset) + p2 * *(value2 + offset)
						+ p3 * *(value3 + offset) + p4 * *(value4 + offset) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_UInt16:
				{
					unsigned short value = p1 * *((unsigned short*)(value1 + offset)) + p2 * *((unsigned short*)(value2 + offset))
						+ p3 * *((unsigned short*)(value3 + offset)) + p4 * *((unsigned short*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_Int16:
				{
					short value = p1 * *((short*)(value1 + offset)) + p2 * *((short*)(value2 + offset))
						+ p3 * *((short*)(value3 + offset)) + p4 * *((short*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_UInt32:
				{
					unsigned int value = p1 * *((unsigned int*)(value1 + offset)) + p2 * *((unsigned int*)(value2 + offset))
						+ p3 * *((unsigned int*)(value3 + offset)) + p4 * *((unsigned int*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_Int32:
				{
					int value = p1 * *((int*)(value1 + offset)) + p2 * *((int*)(value2 + offset))
						+ p3 * *((int*)(value3 + offset)) + p4 * *((int*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_Float32:
				{
					float value = p1 * *((float*)(value1 + offset)) + p2 * *((float*)(value2 + offset))
						+ p3 * *((float*)(value3 + offset)) + p4 * *((float*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				case DT_Float64:
				{
					double value = p1 * *((double*)(value1 + offset)) + p2 * *((double*)(value2 + offset))
						+ p3 * *((double*)(value3 + offset)) + p4 * *((double*)(value4 + offset)) + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				break;

				default:
					break;
				}
			}
		}
	}

	for (auto p : buffers)
	{
		mem_pool_->free(p);
// 		if (p != nullptr)
// 			delete[] p;
	}

	return true;
}