#ifndef HTTPSERVER_DATA_PROCESSOR_H_
#define HTTPSERVER_DATA_PROCESSOR_H_

#include <string>
#include <list>
#include "utility.h"
#include "buffer.h"

class Dataset;
class OGRSpatialReference;
class Style;

class TileProcessor
{
public:
	
	TileProcessor();
	~TileProcessor();

	BufferPtr GetTileData(std::list<std::string> paths, const Envelop& env, int nTileSize, Style* style, const std::string& mimeType);

protected:

	bool SimpleProject(Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height);

	bool DynamicProject(OGRSpatialReference* pDstSpatialReference, Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, unsigned char* pData, unsigned char* pMaskBuffer, int width, int height);

	bool ProcessPerPixel(Dataset* ptrDataset
		, const Envelop& ptrEnvelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, int nBandCount, int bandMap[]
		, unsigned char* memDataOut
		, unsigned char* dataMask
		, RasterResamplingType m_resampType
		, bool bDynProjectToGeoCoord);

};

#endif //HTTPSERVER_DATA_PROCESSOR_H_
