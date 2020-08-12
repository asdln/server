#pragma once

#include <string>
#include <vector>
#include "utility.h"

class DatasetInterface;
class OGRSpatialReference;

class DataProcessor
{
public:
	
	DataProcessor();
	~DataProcessor();

	bool GetData(const std::string& target, void** pData, unsigned long& nDataBytes, std::string& mimeType);

protected:

	bool SimpleProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

	bool DynamicProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

	bool ProcessPerPixel(DatasetInterface* ptrDataset
		, const Envelop& ptrEnvelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, std::vector<int>& vecBands
		, void** memDataOut
		, unsigned char** dataMask
		, RasterResamplingType m_resampType
		, bool bDynProjectToGeoCoord);

	bool Process(DatasetInterface* ptrDataset
		, const Envelop& envelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, std::vector<int>& vecBands
		, void** memDataOut
		, unsigned char** dataMask);

private:

	void* pDstBuffer_ = nullptr;
};

