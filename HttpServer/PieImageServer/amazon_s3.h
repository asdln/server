#ifndef PIEIMAGESERVER_AMAZON_S3_H_
#define PIEIMAGESERVER_AMAZON_S3_H_

#include <string>
#include "buffer.h"

class AmazonS3
{

public:

	static bool GetUseS3() { return s_use_s3_; }

	static void SetUseS3(bool use_s3) { s_use_s3_ = use_s3; }

	bool PutS3Object(const std::string& obj_name, BufferPtr buffer);

	BufferPtr GetS3Object(const std::string& obj_name);

	static void SetBucketName(const std::string& name) { s_bucket_name_ = name; }


protected:

	static std::string s_bucket_name_;

	static bool s_use_s3_;
};

#endif //PIEIMAGESERVER_AMAZON_S3_H_