#include "tile_processor.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "jpg_compress.h"
#include "max_min_stretch.h"
#include "gdal_priv.h"
#include "coordinate_transformation.h"
#include "math.h"
#include <algorithm>
#include "resource_pool.h"
#include "style.h"

template<typename T>
T LinearSample(double u, double v, void* value1, void* value2, void* value3, void* value4, bool bAdjust = true)
{
	double value = (1.0 - u) * (1 - v) * *((T*)value1) + (1.0 - u) * v * *((T*)value2) + u * (1.0 - v) * *((T*)value3) + u * v * *((T*)value4);

	if (bAdjust)
		value += 0.5;

	T tvalue = value;

	return tvalue;
}

bool IsIntersect(const Envelop& ptrMapEnv, const Envelop& ptrLayerEnv, bool bTag = 1)
{
	return false;
}

TileProcessor::TileProcessor()
{
}

bool TileProcessor::ProcessPerPixel(DatasetInterface* ptrDataset
	, const Envelop& ptrEnvelope
	, OGRSpatialReference* ptrVisSRef
	, int nWidth, int nHeight
	, int nBandCount, int bandMap[]
	, void** memDataOut
	, unsigned char** dataMask
	, RasterResamplingType m_resampType
	, bool bDynProjectToGeoCoord)
{
	Envelop ptrEnvDataset = ptrDataset->GetExtent();
	OGRSpatialReference* ptrDSSRef = ptrDataset->GetSpatialReference();

	double dScreenResolutionX = ptrEnvelope.GetWidth() / (double)nWidth;
	double dScreenResolutionY = ptrEnvelope.GetHeight() / (double)nHeight;

	int nRasterWid = ptrDataset->GetRasterXSize();
	int nRasterHei = ptrDataset->GetRasterYSize();

	DataType ePixelType = ptrDataset->GetDataType();
	CoordinateTransformation coordTrans(ptrVisSRef, ptrDSSRef);

	DataType eTpye = ptrDataset->GetDataType();
	int nTypeSize = GetDataTypeBytes(eTpye);
	int nDataPixelBytes = nBandCount * nTypeSize;

	unsigned char* pOutData = new unsigned char[nDataPixelBytes * nWidth * nHeight];
	*memDataOut = pOutData;
	//memDataOut.Attach(new SysDataSource::PixelBuffer(nWidth, nHeight, vecBands, eTpye, pOutData));

	unsigned char* pMaskData = new unsigned char[nWidth * nHeight];
	*dataMask = pMaskData;
	memset(pMaskData, 1, nWidth * nHeight);

	bool bValid = false;

	double dRow = ptrEnvelope.GetYMax();// - dbHalfCellY;
	for (int nRow = 0; nRow < nHeight; nRow++)
	{
		double dCol = ptrEnvelope.GetXMin();
		for (int nCol = 0; nCol < nWidth; nCol++)
		{
			double dx = dCol;
			double dy = dRow;

			//超过地理坐标范围continue
			if (bDynProjectToGeoCoord)
			{
				if (dx < -180 || dx>180 || dy < -90 || dy>90)
				{
					pMaskData[nWidth * nRow + nCol] = 0;
					dCol += dScreenResolutionX;
					continue;
				}
			}

			unsigned char* buffer = pOutData + (nWidth * nRow + nCol) * nDataPixelBytes;
			bool bRes = coordTrans.Transform(1, &dx, &dy);
			if (!bRes || std::isnan(dx) || std::isnan(dy))
			{
				pMaskData[nWidth * nRow + nCol] = 0;
				//memset(buffer, 128, nDataPixelBytes);
			}
			else
			{
				double dx1, dy1;
				ptrDataset->World2Pixel(dx, dy, dx1, dy1);
				int nx1 = dx1;
				int ny1 = dy1;

				if (nx1 < 0 || nx1 >= nRasterWid || ny1 < 0 || ny1 >= nRasterHei)
				{
					pMaskData[nWidth * nRow + nCol] = 0;
				}
				else
				{
					bValid = true;

					if (ePixelType < DataType::DT_CInt16 && m_resampType == RasterResamplingType::BILINEAR_INTERPOLATION && nx1 + 1 < nRasterWid && ny1 + 1 < nRasterHei)
					{
						void* pSrc = nullptr;
						double u = dx1 - nx1;
						double v = dy1 - ny1;

						unsigned char* pLinearBuffer = new unsigned char[4 * nDataPixelBytes];
						ptrDataset->Read(nx1, ny1, 2, 2, pLinearBuffer, 2, 2, eTpye, nBandCount, bandMap);

						unsigned char* value1 = pLinearBuffer;
						unsigned char* value2 = pLinearBuffer + 2 * nDataPixelBytes;
						unsigned char* value3 = pLinearBuffer + nDataPixelBytes;
						unsigned char* value4 = pLinearBuffer + 3 * nDataPixelBytes;

						for (int bandIndex = 0; bandIndex < nBandCount; bandIndex++)
						{
							switch (eTpye)
							{
							case DataType::DT_Byte:
							{
								unsigned char value = LinearSample<unsigned char>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DataType::DT_UInt16:
							{
								unsigned short value = LinearSample<unsigned short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DataType::DT_Int16:
							{
								short value = LinearSample<short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DataType::DT_UInt32:
							{
								unsigned int value = LinearSample<unsigned int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DataType::DT_Int32:
							{
								int value = LinearSample<int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DataType::DT_Float32:
							{
								float value = LinearSample<float>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize, false);
								pSrc = &value;
							}
							break;

							case DataType::DT_Float64:
							{
								double value = LinearSample<double>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize, false);
								pSrc = &value;
							}
							break;

							default:
								break;
							}

							memcpy(buffer + bandIndex * nTypeSize, pSrc, nTypeSize);
						}

						delete[] pLinearBuffer;
						pLinearBuffer = nullptr;
					}
					else
					{
						ptrDataset->Read(nx1, ny1, 1, 1, buffer, 1, 1, eTpye, nBandCount, bandMap);
					}
				}
			}

			dCol += dScreenResolutionX;
		}

		dRow -= dScreenResolutionY;
	}

	return bValid;
}

bool TileProcessor::Process(DatasetInterface* ptrDataset
	, const Envelop& envelope
	, OGRSpatialReference* ptrVisSRef
	, int nWidth, int nHeight
	, int nBandCount, int bandMap[]
	, void** memDataOut
	, unsigned char** dataMask)
{
	const Envelop& ptrEnvDataset = ptrDataset->GetExtent();
	OGRSpatialReference* ptrDSSRef = ptrDataset->GetSpatialReference();

	RasterResamplingType resampType = NEAREST_NEIGHBOR;

	bool bDynPrjTrans = false;

	// 坐标转换
	if (ptrVisSRef && ptrDSSRef)
	{
		if (std::string(ptrVisSRef->GetAttrValue("DATUM")).compare(ptrDSSRef->GetAttrValue("DATUM")) == 0 || !ptrVisSRef->IsSame(ptrDSSRef))
		{
			bDynPrjTrans = true;
		}
		
	}

	bool bDynProjectToGeoCoord = false;
	if (bDynPrjTrans
		&& ptrVisSRef->IsGeographic()
		&& ptrDSSRef->IsProjected())
	{
		bDynProjectToGeoCoord = true;
	}

	Envelop envelopeClone = envelope;
	DataType ePixelType = ptrDataset->GetDataType();

	CoordinateTransformation transformation1(ptrDSSRef, ptrVisSRef);
	CoordinateTransformation transformation2(ptrVisSRef, ptrDSSRef);

	bool bTransRes = false;
	if (bDynPrjTrans)
	{
		bTransRes = transformation2.Transform(envelope, envelopeClone);
		//bTransRes = ((SysGeometry::GeometryPtr)envelopeClone)->Transform(ptrDSSRef);

		double dx1, dy1, dx2, dy2;
		envelopeClone.QueryCoords(dx1, dy1, dx2, dy2);

		//针对极地坐标做特殊处理
		if (!bTransRes || std::isnan(dx1) || std::isnan(dy1) || std::isnan(dx2) || std::isnan(dy2))
		{
			return ProcessPerPixel(ptrDataset, envelope, ptrDSSRef, nWidth, nHeight, nBandCount, bandMap, memDataOut, dataMask, resampType, bDynProjectToGeoCoord);
		}
	}

	double dScreenResolutionX = envelope.GetWidth() / (double)nWidth;
	double dScreenResolutionY = envelope.GetHeight() / (double)nHeight;

	int nRasterWid = ptrDataset->GetRasterXSize();
	int nRasterHei = ptrDataset->GetRasterYSize();

	envelopeClone.Normalize();
	Envelop ptrEnvInterSect;
	bool bInsecResult = ptrEnvDataset.Intersection(envelopeClone, ptrEnvInterSect);
	if (!bInsecResult)
	{
		//确认一下特殊情况转换是否成功，否则进行逐像素转换
		Envelop ptrEvnTemp = envelope;
		if (IsIntersect(ptrEnvDataset, ptrEvnTemp))
		{
			return ProcessPerPixel(ptrDataset, envelope, ptrDSSRef, nWidth, nHeight, nBandCount, bandMap, memDataOut, dataMask, resampType, bDynProjectToGeoCoord);
		}

		return false;
	}

	double dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
	ptrDataset->World2Pixel(ptrEnvInterSect.GetXMin(), ptrEnvInterSect.GetYMin(), dx1, dy1);
	ptrDataset->World2Pixel(ptrEnvInterSect.GetXMin(), ptrEnvInterSect.GetYMax(), dx2, dy2);
	ptrDataset->World2Pixel(ptrEnvInterSect.GetXMax(), ptrEnvInterSect.GetYMax(), dx3, dy3);
	ptrDataset->World2Pixel(ptrEnvInterSect.GetXMax(), ptrEnvInterSect.GetYMin(), dx4, dy4);

	double dLeft = std::min(std::min(dx1, dx2), std::min(dx3, dx4));
	double dRight = std::max(std::max(dx1, dx2), std::max(dx3, dx4));

	double dTop = std::min(std::min(dy1, dy2), std::min(dy3, dy4));
	double dBottom = std::max(std::max(dy1, dy2), std::max(dy3, dy4));

	if (dLeft < 0.0)
		dLeft = 0.0;

	if (dLeft > (double)nRasterWid)
		dLeft = (double)nRasterWid;

	if (dRight < 0.0)
		dRight = 0.0;

	if (dRight > (double)nRasterWid)
		dRight = (double)nRasterWid;

	if (dTop < 0.0)
		dTop = 0.0;

	if (dTop > (double)nRasterHei)
		dTop = (double)nRasterHei;

	if (dBottom < 0.0)
		dBottom = 0.0;

	if (dBottom > (double)nRasterHei)
		dBottom = (double)nRasterHei;

	//计算栅格分辨率
	//double dx11, dy11, dx21, dy21;
	double dxTmp[2];
	double dyTmp[2];

	ptrDataset->Pixel2World(nRasterWid * 0.2, nRasterHei * 0.5, dxTmp[0], dyTmp[0]);
	ptrDataset->Pixel2World(nRasterWid * 0.8, nRasterHei * 0.5, dxTmp[1], dyTmp[1]);

	if (bDynPrjTrans)
	{
		CoordinateTransformation coordTrans(ptrDSSRef, ptrVisSRef);
		coordTrans.Transform(2, dxTmp, dyTmp, nullptr);
	}

	double dImgResX = sqrt((dxTmp[0] - dxTmp[1]) * (dxTmp[0] - dxTmp[1]) + (dyTmp[0] - dyTmp[1]) * (dyTmp[0] - dyTmp[1])) / (nRasterWid * 0.6);

	//查找合适的金字塔级别
	int nPyramidLevel = 0;
	double dScale = 1.0;
	double dResTemp = dImgResX;
	int nMaxLevel = 512;

	if (dImgResX < dScreenResolutionX * 0.5)
	{
		do
		{
			/*if (dResTemp > dScreenResolutionX && dResTemp < dScreenResolutionX * 2.0)*/
			if (dResTemp >= dScreenResolutionX * 0.5 && dResTemp <= dScreenResolutionX)
			{
				break;
			}

			if (nMaxLevel <= 0)
			{
				break;
			}

			dResTemp = dResTemp * 2.0;
			nPyramidLevel++;
			dScale *= 0.5;
			nMaxLevel--;

		} while (1);
	}

	int nSrcWid = std::ceil(dRight) - std::floor(dLeft)/* + 1*/;
	int nSrcHei = std::ceil(dBottom) - std::floor(dTop)/* + 1*/;

	double dDesWid = nSrcWid * dScale/* + 0.5*/;
	double dDesHei = nSrcHei * dScale/* + 0.5*/;

	int nDesWid = std::ceil(dDesWid);
	int nDesHei = std::ceil(dDesHei);

	if (ePixelType < DT_CInt16 && resampType == BILINEAR_INTERPOLATION)
	{
		//双线性重采样，多读一个像素
		nDesWid++;
		nDesHei++;
	}

	nSrcWid = nDesWid / dScale;
	nSrcHei = nDesHei / dScale;

	int nLeft = dLeft;
	int nTop = dTop;

	//放大到高像素时，进行精度补偿
// 		double dOffset = 1.0 / dScale;
// 		int nOff = dOffset;
// 		int nTemp = nLeft / nOff;
// 		nLeft = nTemp * nOff;
// 		nTemp = nTop / nOff;
// 		nTop = nTemp * nOff;

		//如果精度补偿后的位置，不能满足读取的范围，加1个像素进行调整。
		//有可能出现补一个像素不够的情况。暂时加1
	while (nTop + nSrcHei < std::ceil(dBottom))
	{
		nDesHei += 1;
		nSrcHei = nDesHei / dScale;
	}

	while (nLeft + nSrcWid < std::ceil(dRight))
	{
		nDesWid += 1;
		nSrcWid = nDesWid / dScale;
	}

	if (nSrcWid + nLeft > nRasterWid)
	{
		nSrcWid = nRasterWid - nLeft;
		nDesWid = nSrcWid * dScale + 0.5;
	}

	if (nSrcHei + nTop > nRasterHei)
	{
		nSrcHei = nRasterHei - nTop;
		nDesHei = nSrcHei * dScale + 0.5;
	}

	if (nSrcWid <= 0 || nSrcHei <= 0)
	{
		return false;
	}

	if (nDesWid <= 0)
	{
		nDesWid = 1;
	}

	if (nDesHei <= 0)
	{
		nDesHei = 1;
	}

	//SysDataSource::PixelBufferPtr ptrPixelBuffer;

	void* pSrcData = new char[GetDataTypeBytes(ePixelType) * nDesWid * nDesHei * nBandCount];

	//QString pszItem = ptrDataset->GetMetadataItem("INTERLEAVE", "IMAGE_STRUCTURE");
	//if (pszItem == "BAND" || pszItem == "LINE") // 如果是bsq或者bil格式，则按波段分别读取
	//{
	//	SysDataSource::PixelDataType eType = ptrDataset->GetRasterBand(0)->GetRasterDataType();
	//	ptrPixelBuffer.Attach(new SysDataSource::PixelBuffer(nDesWid, nDesHei, vecBands, eType));
	//	char* pData = (char*)(ptrPixelBuffer->GetData());
	//	int nBandCount = vecBands.size();

	//	int nTypeSize = SysDataSource::DataSourceUtil::GetTypeSize(eType);
	//	int nPixelSpace = nTypeSize * nBandCount;
	//	int nLineSpace = nPixelSpace * nDesWid;

	//	for (int i = 0; i < nBandCount; i++)
	//	{
	//		SysDataSource::RasterBandPtr ptrRasterBand = ptrDataset->GetRasterBand(vecBands[i] - 1);
	//		ptrRasterBand->Read(nLeft, nTop, nSrcWid, nSrcHei, pData + i * nTypeSize, nDesWid, nDesHei, eType, nPixelSpace, nLineSpace);

	//		if (ptrTrack && ptrTrack->IsCanced())
	//		{
	//			return false;
	//		}
	//	}
	//}
	//else
	{
		ptrDataset->Read(nLeft, nTop, nSrcWid, nSrcHei, pSrcData, nDesWid, nDesHei, ePixelType, nBandCount, bandMap);
	}


	double dx, dy, dbSrcX, dbSrcY;

	double dImagePosX = 0.0;
	double dImagePosY = 0.0;

	int nTypeSize = GetDataTypeBytes(ePixelType);
	int nDataPixelBytes = nBandCount * nTypeSize;
	unsigned char* pOutData = new unsigned char[nDataPixelBytes * nWidth * nHeight];
	*memDataOut = pOutData;

	//优化显示速度。如果是动态投影，先统一Transform，然后再进行其他操作
	double* pdx = nullptr;
	double* pdy = nullptr;

	double dbHalfCellX = dScreenResolutionX * 0.5;
	double dbHalfCellY = dScreenResolutionY * 0.5;

	if (bDynPrjTrans)
	{
		CoordinateTransformation coordTrans(ptrVisSRef, ptrDSSRef);

		pdx = new double[nWidth * nHeight];
		pdy = new double[nWidth * nHeight];

		dy = envelope.GetYMax() - dbHalfCellY;
		double dXminTemp = envelope.GetXMin();

		for (int RowIndex = 0; RowIndex < nHeight; ++RowIndex)
		{
			dx = dXminTemp + dbHalfCellX;
			for (int ColIndex = 0; ColIndex < nWidth; ++ColIndex)
			{
				pdx[RowIndex * nWidth + ColIndex] = dx;
				pdy[RowIndex * nWidth + ColIndex] = dy;

				dx += dScreenResolutionX;
			}
			dy -= dScreenResolutionY;
		}

		if (!coordTrans.Transform(nWidth * nHeight, pdx, pdy, 0))
		{
			delete[] pdx;
			delete[] pdy;

			delete[] pSrcData;
			return false;
		}
	}

	unsigned char* pMaskData = new unsigned char[nWidth * nHeight];
	*dataMask = pMaskData;
	memset(pMaskData, 1, nWidth * nHeight);

	double ddx1, ddy1, ddx2, ddy2;
	ptrEnvDataset.QueryCoords(ddx1, ddy1, ddx2, ddy2);

	double dScaleX = (nDesWid / (double)nSrcWid);
	double dScaleY = (nDesHei / (double)nSrcHei);

	dy = envelope.GetYMax() - dbHalfCellY;
	for (int RowIndex = 0; RowIndex < nHeight; ++RowIndex)
	{
		dx = envelope.GetXMin() + dbHalfCellX;
		for (int ColIndex = 0; ColIndex < nWidth; ++ColIndex)
		{
			if (bDynPrjTrans)
			{
				dbSrcX = pdx[RowIndex * nWidth + ColIndex];
				dbSrcY = pdy[RowIndex * nWidth + ColIndex];

				if (std::isnan(dbSrcX) || std::isnan(dbSrcY))
				{
					dx += dScreenResolutionX;
					pMaskData[nWidth * RowIndex + ColIndex] = 0;
					continue;
				}
			}
			else
			{
				dbSrcX = dx;
				dbSrcY = dy;
			}
			//超过地理坐标范围continue
			if (bDynProjectToGeoCoord)
			{
				if (dx < -180 || dx>180 || dy < -90 || dy>90)
				{
					dx += dScreenResolutionX;
					pMaskData[nWidth * RowIndex + ColIndex] = 0;
					continue;
				}
			}

			//添加半个像素的补偿，改为全部在范围之外才舍弃。
			//if (dbSrcX < ddx1 || dbSrcX > ddx2 || dbSrcY < ddy1 || dbSrcY > ddy2)
			if (dbSrcX + dbHalfCellX < ddx1 || dbSrcX - dbHalfCellX > ddx2 || dbSrcY + dbHalfCellY < ddy1 || dbSrcY - dbHalfCellY > ddy2)
			{
				pMaskData[nWidth * RowIndex + ColIndex] = 0;
			}
			else
			{
				ptrDataset->World2Pixel(dbSrcX, dbSrcY, dImagePosX, dImagePosY);
				//计算
				double dColPos = (dImagePosX - nLeft) * dScaleX;
				double dRowPos = (dImagePosY - nTop) * dScaleY;

				long nColPos = dColPos;
				long nRowPos = dRowPos;

				if (dImagePosX < 0.0 || dImagePosX > nRasterWid || dImagePosY < 0.0 || dImagePosY > nRasterHei)
				{
					pMaskData[nWidth * RowIndex + ColIndex] = 0;
				}
				//对于变形比较大的投影，例如标称投影或者某些gf4数据，会出现外接范围不能覆盖所需数据的情况，做特殊处理
				else if (nColPos < 0 || nColPos >= nDesWid || nRowPos < 0 || nRowPos >= nDesHei)
				{
					int nImagePosX = dImagePosX;
					int nImagePosY = dImagePosY;

					unsigned char* buffer = new unsigned char[nDataPixelBytes];
					ptrDataset->Read(nImagePosX, nImagePosY, 1, 1, buffer, 1, 1, ePixelType, nBandCount, bandMap);

					unsigned char* pDes = pOutData + (nWidth * RowIndex + ColIndex) * nDataPixelBytes;
					memcpy(pDes, buffer, nDataPixelBytes);
					delete[] buffer;
				}
				else
				{
					void* pSrc = nullptr;
					unsigned char* pDes = pOutData + (nWidth * RowIndex + ColIndex) * nDataPixelBytes;

					if (ePixelType < DT_CInt16 && BILINEAR_INTERPOLATION && nColPos + 1 < nDesWid && nRowPos + 1 < nDesHei)
					{
						double u = dColPos - nColPos;
						double v = dRowPos - nRowPos;

						unsigned char* value1 = (unsigned char*)pSrcData + (nDesWid * nRowPos + nColPos) * nDataPixelBytes;
						unsigned char* value2 = (unsigned char*)pSrcData + (nDesWid * (nRowPos + 1) + nColPos) * nDataPixelBytes;
						unsigned char* value3 = (unsigned char*)pSrcData + (nDesWid * nRowPos + nColPos + 1) * nDataPixelBytes;
						unsigned char* value4 = (unsigned char*)pSrcData + (nDesWid * (nRowPos + 1) + nColPos + 1) * nDataPixelBytes;

						for (int bandIndex = 0; bandIndex < nBandCount; bandIndex++)
						{
							switch (ePixelType)
							{
							case DT_Byte:
							{
								unsigned char value = LinearSample<unsigned char>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_UInt16:
							{
								unsigned short value = LinearSample<unsigned short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_Int16:
							{
								short value = LinearSample<short>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_UInt32:
							{
								unsigned int value = LinearSample<unsigned int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_Int32:
							{
								int value = LinearSample<int>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_Float32:
							{
								float value = LinearSample<float>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							case DT_Float64:
							{
								double value = LinearSample<double>(u, v, value1 + bandIndex * nTypeSize, value2 + bandIndex * nTypeSize, value3 + bandIndex * nTypeSize, value4 + bandIndex * nTypeSize);
								pSrc = &value;
							}
							break;

							default:
								break;
							}

							memcpy(pDes + bandIndex * nTypeSize, pSrc, nTypeSize);
						}
					}
					else
					{
						int nIndex = nDesWid * nRowPos + nColPos;
						int nMaxIndex = nDesWid * nDesHei;
						if (nIndex >= nMaxIndex)
						{
							nIndex = nMaxIndex - 1;
						}

						//由于读取的原始数据金字塔级别的数据，此处进行了重采样。有优化空间？
						pSrc = (unsigned char*)pSrcData + nIndex * nDataPixelBytes;
						memcpy(pDes, pSrc, nDataPixelBytes);
					}
				}
			}
			dx += dScreenResolutionX;
		}
		dy -= dScreenResolutionY;
	}

	if (bDynPrjTrans)
	{
		delete[] pdx;
		delete[] pdy;
	}

	delete[] pSrcData;
	return true;
}

bool TileProcessor::SimpleProject(DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, int width, int height)
{
	double dImgLeft = 0.0;
	double dImgTop = 0.0;

	double dImgRight = 0.0;
	double dImgBottom = 0.0;

	pDataset->World2Pixel(env.GetXMin(), env.GetYMax(), dImgLeft, dImgTop);
	pDataset->World2Pixel(env.GetXMax(), env.GetYMin(), dImgRight, dImgBottom);

	int nRasterWidth = pDataset->GetRasterXSize();
	int nRasterHeight = pDataset->GetRasterYSize();

	DataType DataType = pDataset->GetDataType();

	if (dImgLeft >= nRasterWidth || dImgRight < 0.0 || dImgTop >= nRasterHeight || dImgBottom < 0.0)
	{
		//memset(pData, 255, width * height * nBandCount * GetDataTypeBytes(DataType));
		return false;
	}

	double dAdjustLeft = dImgLeft;
	double dAdjustTop = dImgTop;
	double dAdjustRight = dImgRight;
	double dAdjustBottom = dImgBottom;

	double dBufferWid = width;
	double dBufferHei = height;

	bool bNeedAdjust = false;
	if (dImgLeft < 0.0 || dImgTop < 0.0 || dImgRight >= nRasterWidth || dImgBottom > nRasterHeight)
	{
		dAdjustLeft = dImgLeft < 0.0 ? 0.0 : dImgLeft;
		dAdjustTop = dImgTop < 0.0 ? 0.0 : dImgTop;

		dAdjustRight = dImgRight > nRasterWidth ? nRasterWidth : dImgRight;
		dAdjustBottom = dImgBottom > nRasterHeight ? nRasterHeight : dImgBottom;

		bNeedAdjust = true;
	}

	double srcWidth = dAdjustRight - dAdjustLeft;
	double srcHeight = dAdjustBottom - dAdjustTop;

	if (bNeedAdjust)
	{
		dBufferWid = srcWidth / (dImgRight - dImgLeft) * width;
		dBufferHei = srcHeight / (dImgBottom - dImgTop) * height;
	}

	if (dBufferWid <= 0 || dBufferHei <= 0)
	{
		//memset(pData, 255, width * height * nBandCount * GetDataTypeBytes(DataType));
		return false;
	}

	int nBufferWidth = dBufferWid;
	int nBufferHeight = dBufferHei;

	if (bNeedAdjust)
	{
		int nBufferSize = nBufferWidth * nBufferHeight * GetDataTypeBytes(DataType) * nBandCount;
		char* pTempBuffer = new char[nBufferSize];
		memset(pTempBuffer, 255, nBufferSize);

		pDataset->Read(dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pTempBuffer, dBufferWid, dBufferHei, DataType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);

		double dx = (dImgLeft < 0.0 ? -dImgLeft : 0.0) / (dImgRight - dImgLeft) * width;
		double dy = (dImgTop < 0.0 ? -dImgTop : 0.0) / (dImgBottom - dImgTop) * height;

		//粗略修正一下缝隙问题。精确的接缝问题，需要双线性重采样
		int nx = (int)(dx + dBufferWid + 0.5) - nBufferWidth;
		int ny = (int)(dy + dBufferHei + 0.5) - nBufferHeight;

		if (dImgLeft >= 0.0)
			nx = 0;

		if (dImgTop >= 0.0)
			ny = 0;

		char* pBuffer = (char*)pData;

		int nPixelBytes = GetDataTypeBytes(DataType) * nBandCount;
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
		pDataset->Read(dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pData, nBufferWidth, nBufferHeight, DataType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);
	}

	return true;
}

bool TileProcessor::DynamicProject(OGRSpatialReference* pDstSpatialReference, DatasetInterface* pDataset, int nBandCount, int bandMap[], const Envelop& env, void** pData, int width, int height)
{
	unsigned char* pMaskBuffer = nullptr;
	bool bRes = Process(pDataset, env, pDstSpatialReference, width, height, nBandCount, bandMap, pData, &pMaskBuffer);

	if(pMaskBuffer != nullptr)
		delete[] pMaskBuffer;

	return bRes;

}

bool TileProcessor::GetTileData(std::list<std::string> paths, const Envelop& env, int nTileSize, void** pData, unsigned long& nDataBytes, Style* style, const std::string& mimeType)
{
	//暂时只获取第一个数据集
	std::string filePath = paths.front();

	//OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(
	//	"GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]");

	const char* pWKT = "PROJCS[\"WGS_1984_Web_Mercator\",GEOGCS[\"GCS_WGS_1984_Major_Auxiliary_Sphere\",DATUM[\"WGS_1984_Major_Auxiliary_Sphere\",SPHEROID[\"WGS_1984_Major_Auxiliary_Sphere\",6378137.0,0.0]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"False_Easting\",0.0],PARAMETER[\"False_Northing\",0.0],PARAMETER[\"Central_Meridian\",0.0],PARAMETER[\"latitude_of_origin\",0.0],UNIT[\"Meter\",1.0]]";
	OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(pWKT);

	const size_t nSize = 196608;  // 196608 = 256 * 256 * 3
	//const size_t nSize = 262144;  // 262144 = 256 * 256 * 4
	void* buff = nullptr;

	std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(filePath));

	int nBandCount = 3;
	int bandMap[3] = { 1, 2, 3 };

	OGRSpatialReference* poSpatialReference = tiffDataset->GetSpatialReference();

	bool bSimple = poSpatialReference == nullptr ||
		std::string(pDefaultSpatialReference->GetAttrValue("DATUM")).compare(poSpatialReference->GetAttrValue("DATUM")) == 0 ||
		poSpatialReference->IsSame(pDefaultSpatialReference);

	bool bRes = true;

	if (bSimple)
	{
		buff = new unsigned char[nSize];
		memset(buff, 255, nSize);
		bRes = SimpleProject(tiffDataset.get(), nBandCount, bandMap, env, buff, nTileSize, nTileSize);
	}
	else
	{
		//env 必须是 pDefaultSpatialReference 的空间参考
		bRes = DynamicProject(pDefaultSpatialReference, tiffDataset.get(), nBandCount, bandMap, env, &buff, nTileSize, nTileSize);
	}

	MaxMinStretch stretch;
	stretch.DoStretch(buff, nSize, nBandCount, bandMap, tiffDataset->GetDataType());

	if (!bRes)
	{
		if (buff != nullptr)
			delete[] buff;

		OSRDestroySpatialReference(pDefaultSpatialReference);

		return false;
	}

	if (buff == nullptr)
	{
		buff = new unsigned char[nSize];
		memset(buff, 255, nSize);
	}
	
	JpgCompress jpgCompress;
	jpgCompress.Compress(buff, 256, 256, pData, nDataBytes);

	//test code
	//FILE* pFile = nullptr;
	//std::string path = "d:/test/";

	//int nx, ny, nz;
	//GetTileIndex(target, nx, ny, nz);

	//char string1[32];
	//_itoa_s(nx, string1, 10);
	//path += string1;
	//path += "_";

	//char string2[32];
	//_itoa_s(ny, string2, 10);
	//path += string2;
	//path += "_";

	//char string3[32];
	//_itoa_s(nz, string3, 10);
	//path += string3;

	//path += ".jpg";

	//fopen_s(&pFile, path.c_str(), "wb+");
	//fwrite(pDstBuffer_, 1, nDataBytes, pFile);
	//fclose(pFile);
	//pFile = nullptr;

	if(buff != nullptr)
		delete[] buff;

	OSRDestroySpatialReference(pDefaultSpatialReference);

	return true;
}

TileProcessor::~TileProcessor()
{
}