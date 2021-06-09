#include <iostream>
#include "dataset_factory.h"
#include "s3_dataset.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"
#include <aws/core/Aws.h>

extern Aws::String g_aws_region;

extern Aws::String g_aws_secret_access_key;

extern Aws::String g_aws_access_key_id;

std::shared_ptr<Dataset> DatasetFactory::OpenDataset(const std::string& path)
{
	std::string dst = "/vsis3/";
	if (strncmp(path.c_str(), dst.c_str(), dst.length()) == 0)
	{
		//此处获取需要加锁。外面已经加了锁
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

		auto p = std::make_shared<S3Dataset>();
		if (p->Open(path))
			return p;
		else
		{
			LOG(ERROR) << "can not open data:" << path;
			return nullptr;
		}
	}
	else
	{
		auto p = std::make_shared<TiffDataset>();
		//auto p = std::make_shared<S3Dataset>();
		if (p->Open(path))
			return p;
		else
		{
			LOG(ERROR) << "can not open data:" << path;
			return nullptr;
		}
	}

	return nullptr;
}