#ifndef HTTPSERVER_DATA_PROCESSOR_H_
#define HTTPSERVER_DATA_PROCESSOR_H_

#include <string>
#include <list>
#include "utility.h"

class Dataset;
class OGRSpatialReference;
class Style;

class TileProcessor
{
public:
	
	TileProcessor();
	~TileProcessor();

	bool GetTileData(std::list<std::string> paths, const Envelop& env, int nTileSize, void** pData, unsigned long& nDataBytes, Style* style, const std::string& mimeType);

protected:

	bool SimpleProject(Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

	bool DynamicProject(OGRSpatialReference* pDstSpatialReference, Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, void** pData, unsigned char** pMaskBuffer, int width, int height);

	bool ProcessPerPixel(Dataset* ptrDataset
		, const Envelop& ptrEnvelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, int nBandCount, int bandMap[]
		, void** memDataOut
		, unsigned char** dataMask
		, RasterResamplingType m_resampType
		, bool bDynProjectToGeoCoord);

	bool Process(Dataset* ptrDataset
		, const Envelop& envelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, int nBandCount, int bandMap[]
		, void** memDataOut
		, unsigned char** dataMask);

};

#endif //HTTPSERVER_DATA_PROCESSOR_H_
