#include "dataset_factory.h"
#include "tiff_dataset.h"

std::shared_ptr<DatasetInterface> DatasetFactory::OpenDataset(const std::string& path)
{
	auto p = std::make_shared<TiffDataset>();
	p->Open(path);

	return p;
}