#pragma once

#include <string>
#include "buffer.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

class AmazonS3
{

public:

	static void init();

	static bool GetUseS3() { return s_use_s3_; }

	static void SetUseS3(bool use_s3) { s_use_s3_ = use_s3; }

	static bool PutS3Object(const std::string& obj_name, BufferPtr buffer);

	static BufferPtr GetS3Object(const std::string& obj_name);

	static void SetBucketName(const std::string& s3cachekey);

	static bool CreateBucket();

protected:

	static bool s_use_s3_;

	static Aws::S3::S3Client* s3_client_;

	static Aws::String s3_bucket_name_;

	static Aws::String s3_key_name_;
};