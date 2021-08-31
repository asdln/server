#include "amazon_s3.h"
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include "png_buffer.h"

bool S3Cache::s_use_s3_ = false;

Aws::String S3Cache::s3_bucket_name_;

Aws::String S3Cache::s3_key_name_;

extern Aws::String g_aws_region;

extern Aws::String g_aws_secret_access_key;

extern Aws::String g_aws_access_key_id;

Aws::S3::S3Client* S3Cache::s3_client_ = nullptr;

unsigned char S3Cache::tag_ = 0;

void S3Cache::init()
{
	if (g_aws_region.empty())
	{
		char* AWS_REGION = std::getenv("AWS_REGION");
		g_aws_region = Aws::String(AWS_REGION, strlen(AWS_REGION));
		std::cout << "AWS_REGION : " << g_aws_region << std::endl;
	}

	if (g_aws_secret_access_key.empty())
	{
		char* AWS_SECRET_ACCESS_KEY = std::getenv("AWS_SECRET_ACCESS_KEY");
		g_aws_secret_access_key = Aws::String(AWS_SECRET_ACCESS_KEY, strlen(AWS_SECRET_ACCESS_KEY));
		std::cout << "AWS_SECRET_ACCESS_KEY : " << g_aws_secret_access_key << std::endl;
	}

	if (g_aws_access_key_id.empty())
	{
		char* AWS_ACCESS_KEY_ID = std::getenv("AWS_ACCESS_KEY_ID");
		g_aws_access_key_id = Aws::String(AWS_ACCESS_KEY_ID, strlen(AWS_ACCESS_KEY_ID));
		std::cout << "AWS_ACCESS_KEY_ID : " << g_aws_access_key_id << std::endl;
	}

	Aws::Client::ClientConfiguration config;
	config.region = g_aws_region;

	Aws::Auth::AWSCredentials cred(g_aws_access_key_id, g_aws_secret_access_key);
	s3_client_ = new Aws::S3::S3Client(cred, config);
}

void S3Cache::SetBucketName(const std::string& bucket_name)
{
	if (bucket_name.empty())
	{
		s3_bucket_name_.clear();
		s3_key_name_.clear();
		return;
	}

	std::string s3cachekey_temp = bucket_name;

	//去掉最前面的/和最后面的/	
	{
		int pos = bucket_name.find('/');
		if (pos == 0)
		{
			s3cachekey_temp = bucket_name.substr(1, bucket_name.size() - 1);
		}

		int rpos = bucket_name.rfind('/');
		if (rpos == bucket_name.size() - 1)
		{
			s3cachekey_temp = bucket_name.substr(0, bucket_name.size() - 1);
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
}

bool S3Cache::CreateRootBucket()
{
	Aws::S3::Model::CreateBucketRequest request;
	request.SetBucket(s3_bucket_name_);

	// You only need to set the AWS Region for the bucket if it is 
	// other than US East (N. Virginia) us-east-1.
 	//if (region != Aws::S3::Model::BucketLocationConstraint::us_east_1)
 	{
 		Aws::S3::Model::CreateBucketConfiguration bucket_config;
 		bucket_config.SetLocationConstraint(Aws::S3::Model::BucketLocationConstraint::cn_northwest_1);
 		request.SetCreateBucketConfiguration(bucket_config);
 	}

	Aws::S3::Model::CreateBucketOutcome outcome =
		s3_client_->CreateBucket(request);

	if (!outcome.IsSuccess())
	{
		auto err = outcome.GetError();
		std::cout << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
	}

	return true;
}

bool S3Cache::PutS3Object(const std::string& obj_name, BufferPtr buffer)
{
	Aws::String obj_key = Aws::String(obj_name.c_str(), obj_name.size());
	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(s3_bucket_name_);
	request.SetKey(s3_key_name_ + "/" + obj_key);

	auto sbuff = Aws::New<Aws::Utils::Stream::PreallocatedStreamBuf>("whatever string",
		(unsigned char*)buffer->data(), buffer->size());

	auto sbody = Aws::MakeShared<Aws::IOStream>("whatever string", sbuff);
	request.SetBody(sbody);

	Aws::S3::Model::PutObjectOutcome outcome =
		s3_client_->PutObject(request);

	if (outcome.IsSuccess()) {

		//std::cout << "Added object '" << objectName << "' to bucket '"
		//	<< bucketName << "'" << std::endl;;
		return true;
	}
	else
	{
		std::cout << "ln_debug: Error: PutObject: " <<
			outcome.GetError().GetMessage() << std::endl;

		return false;
	}

	return true;
}

BufferPtr S3Cache::GetS3Object(const std::string& obj_name)
{
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.SetBucket(s3_bucket_name_);
	object_request.SetKey(s3_key_name_ + "/" + Aws::String(obj_name.c_str(), obj_name.size()));

	Aws::S3::Model::GetObjectOutcome get_object_outcome =
		s3_client_->GetObject(object_request);

	if (get_object_outcome.IsSuccess())
	{
		auto& retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
		size_t content_bytes = get_object_outcome.GetResultWithOwnership().GetContentLength();

		auto png_buffer = std::make_shared<PngBuffer>();
		std::vector<unsigned char>& buffer = png_buffer->GetVec();
		buffer.resize(content_bytes);

		//*buffer = new char[content_bytes];
		//*buffer = (char*)pool->malloc(content_bytes);

		retrieved_file.read((char*)buffer.data(), content_bytes);

		//作为读取成功标记，只显示一次。
		if (tag_ == 0)
		{
			tag_++;
			std::cout << "ln_debug: get s3 cache success" << std::endl;
		}

		return png_buffer;
	}
	else
	{
// 		auto err = get_object_outcome.GetError();
// 		std::cout << "Error: GetObject: " <<
// 			err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

		return nullptr;
	}

	return nullptr;
}

bool S3Cache::DeleteObject(const std::string& key)
{
	while(true)
	{
		Aws::S3::Model::ListObjectsRequest request;
		request.WithBucket(s3_bucket_name_);
		Aws::String aws_key = s3_key_name_ + '/' + Aws::String(key.c_str(), key.size());
		request.WithPrefix(std::move(aws_key));

		auto outcome = s3_client_->ListObjects(request);

		Aws::Vector<Aws::S3::Model::ObjectIdentifier> value;
		if (outcome.IsSuccess())
		{
			Aws::Vector<Aws::S3::Model::Object> objects;
			objects = outcome.GetResult().GetContents();

			if(objects.size() == 0)
				break;

			std::cout << "bucket: " << s3_bucket_name_ << "\t" << "key: " << aws_key << "\t" << "size:\t" << objects.size()
				<< std::endl;

			for (Aws::S3::Model::Object& object : objects)
			{
				//std::cout << object.GetKey() << std::endl;
				Aws::S3::Model::ObjectIdentifier identifier;
				identifier.SetKey(object.GetKey());
				value.emplace_back(identifier);
			}

			//return true;
		}
		else
		{
			std::cout << "Error: ListObjects: " <<
				outcome.GetError().GetMessage() << std::endl;

			return false;
		}

		Aws::S3::Model::Delete s3_delete;
		s3_delete.SetObjects(std::move(value));
		Aws::S3::Model::DeleteObjectsRequest delete_request;
		delete_request.SetBucket(s3_bucket_name_);
		delete_request.SetDelete(std::move(s3_delete));

		Aws::S3::Model::DeleteObjectOutcome outcome2 =
			s3_client_->DeleteObjects(delete_request);

		if (!outcome2.IsSuccess())
		{
			auto err = outcome2.GetError();
			std::cout << "Error: DeleteObjects: " <<
				err.GetExceptionName() << ": " << err.GetMessage() << std::endl;

			return false;
		}
		else
		{
			//test code
			std::cout << "s3 delete oOk" << std::endl;
			//return true;
		}
	}

	return true;
}