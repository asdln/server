#pragma once

#include <memory>
#include "dataset.h"

class DatasetFactory
{
public:

	static std::shared_ptr<Dataset> OpenDataset(const std::string& path);
};
