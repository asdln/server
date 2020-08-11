#include "data_processor.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "jpg_compress.h"
#include "max_min_stretch.h"
#include "gdal_priv.h"

DataProcessor::DataProcessor()
{
	pDstBuffer_ = nullptr;
}

bool DataProcessor::SimpleProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height)
{
	double dImgLeft = 0.0;
	double dImgTop = 0.0;

	double dImgRight = 0.0;
	double dImgBottom = 0.0;

	pDataset->World2Pixel(env.dLeft, env.dTop, dImgLeft, dImgTop);
	pDataset->World2Pixel(env.dRight, env.dBottom, dImgRight, dImgBottom);

	int nRasterWidth = pDataset->GetRasterXSize();
	int nRasterHeight = pDataset->GetRasterYSize();

	PIEDataType pieDataType = pDataset->GetDataType();

	if (dImgLeft >= nRasterWidth || dImgRight < 0.0 || dImgTop >= nRasterHeight || dImgBottom < 0.0)
	{
		memset(pData, 255, width * height * nBandCount * GetDataTypeBytes(pieDataType));
		return true;
	}

	double dAdjustLeft = dImgLeft;
	double dAdjustTop = dImgTop;
	double dAdjustRight = dImgRight;
	double dAdjustBottom = dImgBottom;

	int nBufferWidth = width;
	int nBufferHeight = height;

	bool bNeedAdjust = false;
	if (dImgLeft < 0.0 || dImgTop < 0.0 || dImgRight >= nRasterWidth || dImgBottom > nRasterHeight)
	{
		dAdjustLeft = dImgLeft < 0.0 ? 0.0 : dImgLeft;
		dAdjustTop = dImgTop < 0.0 ? 0.0 : dImgTop;

		dAdjustRight = dImgRight > nRasterWidth ? nRasterWidth : dImgRight;
		dAdjustBottom = dImgBottom > nRasterHeight ? nRasterHeight : dImgBottom;

		bNeedAdjust = true;
	}

	int srcWidth = dAdjustRight - dAdjustLeft;
	int srcHeight = dAdjustBottom - dAdjustTop;

	if (bNeedAdjust)
	{
		nBufferWidth = srcWidth / (dImgRight - dImgLeft) * width;
		nBufferHeight = srcHeight / (dImgBottom - dImgTop) * height;
	}

	if (bNeedAdjust)
	{
		int nBufferSize = nBufferWidth * nBufferHeight * GetDataTypeBytes(pieDataType) * nBandCount;
		char* pTempBuffer = new char[nBufferSize];
		memset(pTempBuffer, 255, nBufferSize);

		pDataset->Read(dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pTempBuffer, nBufferWidth, nBufferHeight, pieDataType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);

		int nx = (dImgLeft < 0.0 ? -dImgLeft : 0.0) / (dImgRight - dImgLeft) * width;
		int ny = (dImgTop < 0.0 ? -dImgTop : 0.0) / (dImgBottom - dImgTop) * height;

		char* pBuffer = (char*)pData;

		int nPixelBytes = GetDataTypeBytes(pieDataType) * nBandCount;
		int nSrcLineBytes = nBufferWidth * nPixelBytes;
		for (int j = nBufferHeight - 1; j >= 0; j--)
		{
			char* pSrcLine = pTempBuffer + j * nSrcLineBytes;
			char* pDstLine = pBuffer + ((j + ny) * width + nx) * nPixelBytes;

			memcpy(pDstLine, pSrcLine, nSrcLineBytes);
			//memset(pSrcLine, 0, nSrcLineBytes);
		}

		//memcpy(pData, pTempBuffer, width * height * GetPixelDataTypeSize(pixelType));
		delete[] pTempBuffer;
	}
	else
	{
		pDataset->Read(dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pData, nBufferWidth, nBufferHeight, pieDataType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);
	}

	return true;
}

bool DataProcessor::DynamicProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height)
{
	return true;

}

bool DataProcessor::GetData(const std::string& target, void** pData, unsigned long& nDataBytes, std::string& mimeType)
{
	Envelop env;
	std::string filePath = "";

	//OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(
	//	"GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]");

	ParseURL(target, env, filePath);

	const size_t nSize = 196608;  // 196608 = 256 * 256 * 3
	//const size_t nSize = 262144;  // 262144 = 256 * 256 * 4
	unsigned char buff[nSize];
	memset(buff, 255, nSize);

	//后期添加文件句柄缓存
	int nTileSize = 256;
	TiffDataset tiffDataset;
	tiffDataset.Open(filePath);

	int nBandCount = 3;
	int bandMap[3] = { 1, 2, 3 };

	OGRSpatialReference* poSpatialReference = tiffDataset.GetSpatialReference();
	if (1/*poSpatialReference == nullptr || poSpatialReference->IsSameGeogCS(pDefaultSpatialReference)*/)
	{
		SimpleProject(&tiffDataset, nBandCount, bandMap, env, buff, nTileSize, nTileSize);
	}
	else
	{
		DynamicProject(&tiffDataset, nBandCount, bandMap, env, buff, nTileSize, nTileSize);
	}

	MaxMinStretch stretch;
	stretch.DoStretch(buff, nSize, nBandCount, bandMap, tiffDataset.GetDataType());
	
	if (pDstBuffer_ != nullptr)
	{
		free(pDstBuffer_);
		pDstBuffer_ = nullptr;
	}
	
	JpgCompress jpgCompress;
	jpgCompress.Compress(buff, 256, 256, &pDstBuffer_, nDataBytes);

	*pData = pDstBuffer_;

	//OSRDestroySpatialReference(pDefaultSpatialReference);

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