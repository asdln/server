#ifndef HTTPSERVER_DATASET_FACTORY_H_
#define HTTPSERVER_DATASET_FACTORY_H_

#include <memory>
#include "dataset.h"

class DatasetFactory
{
public:

	static std::shared_ptr<Dataset> OpenDataset(const std::string& path);
};

#endif //HTTPSERVER_DATASET_FACTORY_H_
