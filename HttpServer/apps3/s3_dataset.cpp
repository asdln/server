#include "s3_dataset.h"
#include <iostream>
#include <fstream>
#include <array>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/auth/AWSCredentials.h>
#include "gdal_priv.h"

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
			err.GetExceptionName() << ": " << err.GetMessage() << "\t" << objectKey << std::endl;

		return false;
	}
}

bool AWSS3GetObject2(Aws::S3::S3Client* s3_client, const Aws::String& fromBucket
	, const Aws::String& objectKey,
	std::vector<unsigned char>& buffer)
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

		std::cout << "bucket: " << fromBucket << "\t" << "key: " << objectKey << std::endl;

		return false;
	}
}

bool AWSS3PutObject2(Aws::S3::S3Client* s3_client, const Aws::String& bucketName
	, const Aws::String& objectName
	, std::vector<unsigned char>& buffer)
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

		std::cout << "bucket: " << bucketName << "\t" << "key: " << objectName << std::endl;

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
		if (s3_client_ == nullptr)
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

template<typename T>
void LinearSampleFromBlock(int nx, int ny, int index_left, int index_top, int index_right, int index_bottom, int width
	, int height, int bufferWidth, int bufferHeight, int nTypeSize, double scale0, const std::vector<unsigned char* >& buffers
	, int pixel_bytes_bin, void* pData, int type_bytes, int nBandCount, int* pBandMap, int have_nodata, double nodata_value)
{
	int block_count_x = index_right - index_left + 1;
	int block_count_y = index_bottom - index_top + 1;

	int level_left = index_left * 256;
	int level_top = index_top * 256;

	double res_x = (double)width / bufferWidth;
	double res_y = (double)height / bufferHeight;

	double mul = 1.0 / scale0;
	for (int j = 0; j < bufferHeight; j++)
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

			unsigned char* pDes = (unsigned char*)pData + (bufferWidth * j + i) * type_bytes * nBandCount;
			double p1 = (1.0 - u) * (1.0 - v);
			double p2 = (1.0 - u) * v;
			double p3 = u * (1.0 - v);
			double p4 = u * v;

			for (int band = 0; band < nBandCount; band++)
			{
				int bandIndex = pBandMap[band] - 1;
				int offset = bandIndex * nTypeSize;

				const T& v1 = *((T*)(value1 + offset));
				const T& v2 = *((T*)(value2 + offset));
				const T& v3 = *((T*)(value3 + offset));
				const T& v4 = *((T*)(value4 + offset));

				//如果有无效值，则按照最邻近采样，解决黑边问题
				if (have_nodata && (v1 == nodata_value || v2 == nodata_value || v3 == nodata_value || v4 == nodata_value))
				{
					int U = u + 0.5;
					int V = v + 0.5;

					std::array<T, 4> arr = { v1, v3, v2, v4 };
					T value = arr.at(V * 2 + U);
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
				else
				{
					T value = p1 * v1 + p2 * v2 + p3 * v3 + p4 * v4 + 0.5;
					memcpy(pDes + band * nTypeSize, &value, nTypeSize);
				}
			}
		}
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
		long long time_now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		//20秒
		if (time_now - time_for_change_ >= 20)
		{
			switch_ = true;
			std::cout << "change s3 pyramid check status to true" << std::endl;
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
		
		if (scale >= min && scale <= max || scale < 2.0)
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

				std::cout << "error: can not load s3 tile data, use origin tiff instead" << std::endl; 
				s3_failed_count_++;

				//连续4次读取不成功，则认为没有s3金字塔
				if (s3_failed_count_ >= 4)
				{
					s3_failed_count_ = 0;
					switch_ = false;
					time_for_change_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					std::cout << "change s3 pyramid check status to false" << std::endl;
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

	int have_nodata;
	double nodata_value;
	nodata_value = poDataset_->GetRasterBand(1)->GetNoDataValue(&have_nodata);

	switch (dataType)
	{
	case DT_Byte:
	{
		LinearSampleFromBlock<unsigned char>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_UInt16:
	{
		LinearSampleFromBlock<unsigned short>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_Int16:
	{
		LinearSampleFromBlock<short>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_UInt32:
	{
		LinearSampleFromBlock<unsigned int>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_Int32:
	{
		LinearSampleFromBlock<int>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_Float32:
	{
		LinearSampleFromBlock<float>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	case DT_Float64:
	{
		LinearSampleFromBlock<double>(nx, ny, index_left, index_top, index_right
			, index_bottom, width, height, bufferWidth, bufferHeight, nTypeSize, scale0
			, buffers, pixel_bytes_bin, pData, type_bytes_, nBandCount, pBandMap, have_nodata, nodata_value);
	}
	break;

	default:
		break;
	}

	for (auto p : buffers)
	{
		mem_pool_->free(p);
// 		if (p != nullptr)
// 			delete[] p;
	}

	return true;
}

bool S3Dataset::ReadHistogramFile(std::string& file_content)
{
	std::string str_prefix = ".pyra/histogram.json";

	Aws::String key_name = s3_key_name_;
	key_name += str_prefix;

	std::vector<unsigned char> buffer;
	if (AWSS3GetObject2(s3_client_, s3_bucket_name_, key_name, buffer))
	{
		std::string string_json(buffer.begin(), buffer.end());
		file_content = string_json;
		return true;
	}

	return false;
}

bool S3Dataset::SaveHistogramFile(const std::string& file_content)
{
	std::string str_prefix = ".pyra/histogram.json";

	Aws::String key_name = s3_key_name_;
	key_name += str_prefix;

	std::vector<unsigned char> buffer;
	buffer.assign(file_content.begin(), file_content.end());

	if (AWSS3PutObject2(s3_client_, s3_bucket_name_, key_name, buffer))
	{
		return true;
	}

	return false;
}