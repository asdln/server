#include "tiff_dataset.h"

#include "gdal_priv.h"

void TiffDataset::Open(const std::string& path)
{
	GDALAllRegister();
	poDataset_ = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
}

void TiffDataset::Close()
{
	GDALClose(poDataset_);
	poDataset_ = nullptr;
}

bool TiffDataset::Projection2ImageRowCol(double* adfGeoTransform, double dProjX, double dProjY, double& dCol, double& dRow)
{
	try
	{
		double dTemp = adfGeoTransform[1] * adfGeoTransform[5] - adfGeoTransform[2] * adfGeoTransform[4];
		
		dCol = (adfGeoTransform[5] * (dProjX - adfGeoTransform[0]) -
			adfGeoTransform[2] * (dProjY - adfGeoTransform[3])) / dTemp;
		dRow = (adfGeoTransform[1] * (dProjY - adfGeoTransform[3]) -
			adfGeoTransform[4] * (dProjX - adfGeoTransform[0])) / dTemp;

		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool TiffDataset::Read(const Envelop& env, void* pData, int width, int height, PixelDataType pixelType)
{
	double dImgLeft = 0.0;
	double dImgTop = 0.0;

	double dImgRight = 0.0;
	double dImgBottom = 0.0;

	double dGeoTransform[6];
	poDataset_->GetGeoTransform(dGeoTransform);

	Projection2ImageRowCol(dGeoTransform, env.dLeft, env.dTop, dImgLeft, dImgTop);
	Projection2ImageRowCol(dGeoTransform, env.dRight, env.dBottom, dImgRight, dImgBottom);

	int nRasterWidth = poDataset_->GetRasterXSize();
	int nRasterHeight = poDataset_->GetRasterYSize();

	if (dImgLeft >= nRasterWidth || dImgRight < 0.0 || dImgTop >= nRasterHeight || dImgBottom < 0.0)
	{
		memset(pData, 255, width * height * GetPixelDataTypeSize(pixelType));
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

	GDALDataType eType = GDT_Byte;

	int nBandCount = 3;
	int bandMap[3] = { 1, 2, 3 };
	
	if (bNeedAdjust)
	{
		char* pTempBuffer = new char[nBufferWidth * nBufferHeight * GetPixelDataTypeSize(pixelType)];
		memset(pTempBuffer, 255, nBufferWidth * nBufferHeight * GetPixelDataTypeSize(pixelType));

		poDataset_->RasterIO(GF_Read, dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pTempBuffer, nBufferWidth, nBufferHeight, eType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);

		int nx = (dImgLeft < 0.0 ? -dImgLeft : 0.0) / (dImgRight - dImgLeft) * width;
		int ny = (dImgTop < 0.0 ? -dImgTop : 0.0) / (dImgBottom - dImgTop) * height;

		char* pBuffer = (char*)pData;

		int nPixelBytes = GDALGetDataTypeSize(eType) / 8 * nBandCount;
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
		poDataset_->RasterIO(GF_Read, dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pData, nBufferWidth, nBufferHeight, eType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);
	}
	
	return true;
}