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
			adfGeoTransform[2] * (dProjY - adfGeoTransform[3])) / dTemp + 0.5;
		dRow = (adfGeoTransform[1] * (dProjY - adfGeoTransform[3]) -
			adfGeoTransform[4] * (dProjX - adfGeoTransform[0])) / dTemp + 0.5;

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
		memset(poDataset_, 255, width * height * GetPixelDataTypeSize(pixelType));
		return true;
	}

	int nImgLeft = dImgLeft;
	int nImgTop = dImgTop;
	int nImgRight = dImgRight;
	int nImgBottom = dImgBottom;

	int nBufferWidth = width;
	int nBufferHeight = height;

	bool bNeedAdjust = false;
	if (dImgLeft < 0.0 || dImgTop < 0.0 || dImgRight >= nRasterWidth || dImgBottom > nRasterHeight)
	{
		nImgLeft = dImgLeft < 0.0 ? 0.0 : dImgLeft;
		nImgTop = dImgTop < 0.0 ? 0.0 : dImgTop;
		
		nImgRight = dImgRight >= nRasterWidth ? nRasterWidth - 1 : dImgRight;
		nImgBottom = dImgBottom >= nRasterHeight ? nRasterHeight - 1 : dImgBottom;

		bNeedAdjust = true;
	}

	int srcWidth = nImgRight - nImgLeft + 1;
	int srcHeight = nImgBottom - nImgTop + 1;

	if (bNeedAdjust)
	{
		nBufferWidth = srcWidth / (dImgRight - dImgLeft) * width;
		nBufferHeight = srcHeight / (dImgBottom - dImgTop) * height;
	}

	GDALDataType eType = GDT_Byte;

	int nBandCount = 3;
	int bandMap[3] = { 1, 2, 3 };
	CPLErr err = poDataset_->RasterIO(GF_Read, nImgLeft, nImgTop, srcWidth, srcHeight, pData, nBufferWidth, nBufferHeight, eType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);

	if (bNeedAdjust)
	{
		int nx = (dImgLeft < 0.0 ? -dImgLeft : 0.0) / (dImgRight - dImgLeft) * width;
		int ny = (dImgTop < 0.0 ? -dImgTop : 0.0) / (dImgBottom - dImgTop) * height;

		char* pBuffer = (char*)pData;
		//char* pTempBuffer = new char[width * height * GetPixelDataTypeSize(pixelType)];

		int nPixelBytes = GDALGetDataTypeSize(eType) / 8 * nBandCount;
		int nSrcLineBytes = nBufferWidth * nPixelBytes;
		for (int j = nBufferHeight - 1; j >= 0; j--)
		{
			char* pSrcLine = pBuffer + j * nSrcLineBytes;
			char* pDstLine = pBuffer + ((j + ny) * width + nx) * nPixelBytes;

			memcpy(pDstLine, pSrcLine, nSrcLineBytes);
			memset(pSrcLine, 255, nSrcLineBytes);
		}

		//memcpy(pData, pTempBuffer, width * height * GetPixelDataTypeSize(pixelType));
		//delete[] pTempBuffer;
	}
	
	return true;
}