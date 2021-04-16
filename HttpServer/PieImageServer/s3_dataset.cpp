#include "s3_dataset.h"
#include<iostream>
#include <fstream>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/auth/AWSCredentials.h>

Aws::String g_aws_region;

Aws::String g_aws_secret_access_key;

Aws::String g_aws_access_key_id;

template<typename T>
T LinearSample(double u, double v, void* value1, void* value2, void* value3, void* value4, bool bAdjust = true)
{
	double value = (1.0 - u) * (1 - v) * *((T*)value1) + (1.0 - u) * v * *((T*)value2) + u * (1.0 - v) * *((T*)value3) + u * v * *((T*)value4);

	if (bAdjust)
		value += 0.5;

	T tvalue = value;

	return tvalue;
}

bool AWSS3GetObject(const Aws::String& fromBucket, const Aws::String& objectKey
	, std::vector<unsigned char>& buffer)
{
	Aws::Client::ClientConfiguration config;


	if (g_aws_access_key_id.empty())
	{
		std::cout << "error: getenv(\"AWS_ACCESS_KEY_ID\")" << g_aws_access_key_id << std::endl;
	}

	config.region = g_aws_region;

	Aws::Auth::AWSCredentials cred(g_aws_access_key_id, g_aws_secret_access_key);
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

		if (buffer.size() < content_bytes)
		{
			buffer.resize(content_bytes);
		}

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

void LoadTileData(const std::string& path, size_t x, size_t y, size_t z, std::vector<unsigned char>& buffer)
{
	std::string str_prefix = ".pyra/" + std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z) + ".bin";
	std::string new_path = path + str_prefix;

#ifdef USE_FILE

	std::ifstream inFile2(new_path, std::ios::binary | std::ios::in);
	inFile2.seekg(0, std::ios_base::end);
	int FileSize2 = inFile2.tellg();

	inFile2.seekg(0, std::ios::beg);

	//char* buffer2 = new char[FileSize2];
	buffer.resize(FileSize2);
	inFile2.read((char*)buffer.data(), FileSize2);
	inFile2.close();

#else

	std::string temp_str = path;
	if (temp_str.rfind("/vsis3/", 0) != 0)
	{
		std::cout << "error: path not begin with \"/vsis3/\"" << std::endl;
		return;
	}

	temp_str = std::string(temp_str.c_str() + 7, temp_str.size() - 7);
	int pos = temp_str.find("/");

	Aws::String src_bucket_name = Aws::String(temp_str.c_str(), pos);
	Aws::String src_key_name = Aws::String(temp_str.c_str() + pos + 1, temp_str.size() - pos - 1);
	src_key_name += str_prefix;

	AWSS3GetObject(src_bucket_name, src_key_name, buffer);

#endif // USE_FILE

}

bool S3Dataset::Open(const std::string& path)
{
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
}

bool S3Dataset::Read(int nx, int ny, int width, int height,void* pData, int bufferWidth
	, int bufferHeight, DataType dataType, int nBandCount, int* pBandMap
	, long long pixSpace , long long lineSapce, long long bandSpace, void* psExtraArg)
{
	int level = 1;
	double scale = std::min((double)width / bufferWidth, (double)height / bufferHeight);
	if (scale <= 2.0)
	{
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

	std::vector<std::vector<unsigned char>> buffers;

	for (int n = index_top; n <= index_bottom; n++)
	{
		for (int m = index_left; m <= index_right; m++)
		{
			std::vector<unsigned char> buffer;
			LoadTileData(file_path_, m, n, level, buffer);
			buffers.push_back(buffer);
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
				unsigned char* src = buffers[block_index].data();

				src = src + (sample_y[w] * 256 + sample_x[w]) * pixel_bytes_bin;
				src_data[w] = src;
			}

			unsigned char* value1 = src_data[0];
			unsigned char* value2 = src_data[1];
			unsigned char* value3 = src_data[2];
			unsigned char* value4 = src_data[3];

			void* pSrc = nullptr;
			unsigned char* pDes = (unsigned char*)pData + (bufferWidth * j + i) * type_bytes_ * nBandCount;

			for (int band = 0; band < nBandCount; band++)
			{
				int bandIndex = pBandMap[band] - 1;
				switch (dataType)
				{
				case DT_Byte:
				{
					unsigned char value = LinearSample<unsigned char>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_UInt16:
				{
					unsigned short value = LinearSample<unsigned short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_Int16:
				{
					short value = LinearSample<short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_UInt32:
				{
					unsigned int value = LinearSample<unsigned int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_Int32:
				{
					int value = LinearSample<int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_Float32:
				{
					float value = LinearSample<float>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				case DT_Float64:
				{
					double value = LinearSample<double>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
					pSrc = &value;
				}
				break;

				default:
					break;
				}

				memcpy(pDes + band * nTypeSize, pSrc, nTypeSize);
			}
		}
	}

	return true;
}