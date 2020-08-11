#include "data_processor.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "jpg_compress.h"

#include "gdal_priv.h"

DataProcessor::DataProcessor()
{
	pDstBuffer_ = nullptr;
}

bool DataProcessor::GetData(const std::string& target, void** pData, unsigned long& nDataBytes, std::string& mimeType)
{
	Envelop env;
	std::string filePath = "";

	OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(
		"GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]");

	GetInfo(target, env, filePath);

	const size_t nSize = 196608;  // 196608 = 256 * 256 * 3
	//const size_t nSize = 262144;  // 262144 = 256 * 256 * 4
	unsigned char buff[nSize];
	memset(buff, 255, nSize);

	int nTileSize = 256;
	TiffDataset tiffDataset;
	tiffDataset.Open(filePath);
	tiffDataset.Read(env, buff, nTileSize, nTileSize, PIXEL_TYPE_RGB);

	if (pDstBuffer_ != nullptr)
	{
		free(pDstBuffer_);
		pDstBuffer_ = nullptr;
	}
	
	JpgCompress jpgCompress;
	jpgCompress.Compress(buff, 256, 256, &pDstBuffer_, nDataBytes);

	*pData = pDstBuffer_;

	OSRDestroySpatialReference(pDefaultSpatialReference);

	return true;
}

DataProcessor::~DataProcessor()
{
	if (pDstBuffer_ != nullptr)
	{
		free(pDstBuffer_);
		pDstBuffer_ = nullptr;
	}
}