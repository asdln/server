#include "dataset_factory.h"
#include "tiff_dataset.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

std::shared_ptr<Dataset> DatasetFactory::OpenDataset(const std::string& path)
{
	auto p = std::make_shared<TiffDataset>();
	if (p->Open(path))
		return p;
	else
	{
		return nullptr;
		LOG(ERROR) << "can not open data:"<< path;
	}
}