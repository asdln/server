#include "amazon_s3.h"

bool AmazonS3::s_use_s3_ = false;

std::string AmazonS3::s_bucket_name_ = "";

bool AmazonS3::PutS3Object(const std::string& obj_name, BufferPtr buffer)
{
	return true;
}

BufferPtr AmazonS3::GetS3Object(const std::string& obj_name)
{

	return nullptr;
}