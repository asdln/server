#include "dataset_factory.h"
#include "s3_dataset.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

std::shared_ptr<Dataset> DatasetFactory::OpenDataset(const std::string& path)
{
	std::string dst = "/vsis3/";
	if (strncmp(path.c_str(), dst.c_str(), dst.length()) == 0)
	{
		auto p = std::make_shared<S3Dataset>();
		if (p->Open(path))
			return p;
		else
		{
			return nullptr;
			LOG(ERROR) << "can not open data:" << path;
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
			return nullptr;
			LOG(ERROR) << "can not open data:" << path;
		}
	}
}