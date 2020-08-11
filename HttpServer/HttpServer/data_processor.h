#pragma once

#include <string>
#include "utility.h"

class DatasetInterface;

class DataProcessor
{
public:
	
	DataProcessor();
	~DataProcessor();

	bool GetData(const std::string& target, void** pData, unsigned long& nDataBytes, std::string& mimeType);

protected:

	bool SimpleProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

	bool DynamicProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

private:

	void* pDstBuffer_ = nullptr;
};

