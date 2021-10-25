#pragma once

#include "tiff_dataset.h"

#include <chrono>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

class S3Dataset : public TiffDataset
{
public:

	virtual bool Open(const std::string& path) override;

	virtual bool Read(int nx, int ny, int width, int height,
		void* pData, int bufferWidth, int bufferHeight, DataType dataType,
		int nBandCount, int* pBandMap, long long pixSpace = 0, long long lineSapce = 0, long long bandSpace = 0,
		void* psExtraArg = nullptr) override;

	void SetS3CacheKey(const std::string& s3cachekey);

	void SetS3Client(Aws::S3::S3Client* s3_client) { s3_client_ = s3_client; }

	virtual bool ReadHistogramFile(std::string& file_content) override;

	virtual bool SaveHistogramFile(const std::string& file_content) override;

	~S3Dataset() { if (s3_client_) delete s3_client_; }

protected:

	std::vector<std::pair<size_t, size_t>> dims_;

	int type_bytes_ = 1;

	Aws::S3::S3Client* s3_client_ = nullptr;

	Aws::String s3_bucket_name_;

	Aws::String s3_key_name_;

	bool switch_ = false;

	int s3_failed_count_ = 0;

	long long time_for_change_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
};