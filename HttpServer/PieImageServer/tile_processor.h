#pragma once

#include <string>
#include <list>
#include "utility.h"
#include "buffer.h"
#include "dataset.h"
#include "style.h"

class OGRSpatialReference;

class TileProcessor
{
public:
	
	TileProcessor();
	~TileProcessor();

	static bool GetTileData(Dataset* dataset, Style* style, const Envelop& env, int tile_width, int tile_height, unsigned char** buff, unsigned char** mask_buffer, int render_color_count);

	static BufferPtr GetCombinedData(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height);

protected:

	static bool SimpleProject(Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, unsigned char* pMaskData, int width, int height);

	static bool DynamicProject(OGRSpatialReference* pDstSpatialReference, Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, unsigned char* pData, unsigned char* pMaskBuffer, int width, int height);

	static bool ProcessPerPixel(Dataset* ptrDataset
		, const Envelop& ptrEnvelope
		, OGRSpatialReference* ptrVisSRef
		, int nWidth, int nHeight
		, int nBandCount, int bandMap[]
		, unsigned char* memDataOut
		, unsigned char* dataMask
		, RasterResamplingType m_resampType
		, bool bDynProjectToGeoCoord);

};
