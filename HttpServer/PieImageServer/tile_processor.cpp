#include "tile_processor.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "jpg_compress.h"
#include "png_compress.h"
#include "webp_compress.h"
#include "min_max_stretch.h"
#include "gdal_priv.h"
#include "coordinate_transformation.h"
#include "math.h"
#include <algorithm>
#include "resource_pool.h"
#include "style.h"

bool IsIntersect(const Envelop& ptrMapEnv, const Envelop& ptrLayerEnv, bool bTag = 1)
{
	return false;
}

TileProcessor::TileProcessor()
{
}

bool TileProcessor::ProcessPerPixel(Dataset* ptrDataset
	, const Envelop& ptrEnvelope
	, OGRSpatialReference* ptrVisSRef
	, int nWidth, int nHeight
	, int nBandCount, int bandMap[]
	, unsigned char* pOutData
	, unsigned char* pMaskData
	, RasterResamplingType m_resampType
	, bool bDynProjectToGeoCoord)
{
	Envelop ptrEnvDataset = ptrDataset->GetExtent();
	OGRSpatialReference* ptrDSSRef = ptrDataset->GetSpatialReference();
	MemoryPool* pool = ptrDataset->GetMemoryPool();

	double dScreenResolutionX = ptrEnvelope.GetWidth() / (double)nWidth;
	double dScreenResolutionY = ptrEnvelope.GetHeight() / (double)nHeight;

	int nRasterWid = ptrDataset->GetRasterXSize();
	int nRasterHei = ptrDataset->GetRasterYSize();

	DataType ePixelType = ptrDataset->GetDataType();
	CoordinateTransformation coordTrans(ptrVisSRef, ptrDSSRef);

	DataType eTpye = ptrDataset->GetDataType();
	int nTypeSize = GetDataTypeBytes(eTpye);
	int nDataPixelBytes = nBandCount * nTypeSize;

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
						//双线性重采样，需要修正0.5个像素。举例：0.8按照双线性，第二个像素占0.8权重，第一个像素占0.2权重
						//实际应该是第一个像素占1.0-(0.8-0.5)=0.7,第二个像素占0.3

						dx1 -= 0.5;
						dy1 -= 0.5;

						if (dx1 < 0.0)
							dx1 = 0.0;

						if (dy1 < 0.0)
							dy1 = 0.0;

						nx1 = dx1;
						ny1 = dy1;

						void* pSrc = nullptr;
						double u = dx1 - nx1;
						double v = dy1 - ny1;

						//unsigned char* pLinearBuffer = new unsigned char[4 * nDataPixelBytes];
						unsigned char* pLinearBuffer = pool->malloc(4 * nDataPixelBytes);
						ptrDataset->Read(nx1, ny1, 2, 2, pLinearBuffer, 2, 2, eTpye, nBandCount, bandMap);

						unsigned char* value1 = pLinearBuffer;
						unsigned char* value2 = pLinearBuffer + 2 * nDataPixelBytes;
						unsigned char* value3 = pLinearBuffer + nDataPixelBytes;
						unsigned char* value4 = pLinearBuffer + 3 * nDataPixelBytes;

						double p1 = (1.0 - u) * (1.0 - v);
						double p2 = (1.0 - u) * v;
						double p3 = u * (1.0 - v);
						double p4 = u * v;

						for (int bandIndex = 0; bandIndex < nBandCount; bandIndex++)
						{
							int offset = bandIndex * nTypeSize;

							switch (eTpye)
							{
							case DT_Byte:
							{
								unsigned char value = p1 * *(value1 + offset) + p2 * *(value2 + offset)
									+ p3 * *(value3 + offset) + p4 * *(value4 + offset) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_UInt16:
							{
								unsigned short value = p1 * *((unsigned short*)(value1 + offset)) + p2 * *((unsigned short*)(value2 + offset))
									+ p3 * *((unsigned short*)(value3 + offset)) + p4 * *((unsigned short*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_Int16:
							{
								short value = p1 * *((short*)(value1 + offset)) + p2 * *((short*)(value2 + offset))
									+ p3 * *((short*)(value3 + offset)) + p4 * *((short*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_UInt32:
							{
								unsigned int value = p1 * *((unsigned int*)(value1 + offset)) + p2 * *((unsigned int*)(value2 + offset))
									+ p3 * *((unsigned int*)(value3 + offset)) + p4 * *((unsigned int*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_Int32:
							{
								int value = p1 * *((int*)(value1 + offset)) + p2 * *((int*)(value2 + offset))
									+ p3 * *((int*)(value3 + offset)) + p4 * *((int*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_Float32:
							{
								float value = p1 * *((float*)(value1 + offset)) + p2 * *((float*)(value2 + offset))
									+ p3 * *((float*)(value3 + offset)) + p4 * *((float*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							case DT_Float64:
							{
								double value = p1 * *((double*)(value1 + offset)) + p2 * *((double*)(value2 + offset))
									+ p3 * *((double*)(value3 + offset)) + p4 * *((double*)(value4 + offset)) + 0.5;
								memcpy(buffer + offset, &value, nTypeSize);
							}
							break;

							default:
								break;
							}
						}

						pool->free(pLinearBuffer);
// 						delete[] pLinearBuffer;
// 						pLinearBuffer = nullptr;
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

bool TileProcessor::SimpleProject(Dataset* pDataset, int nBandCount, int bandMap[], const Envelop& env, void* pData, unsigned char* pMaskData, int width, int height)
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

	if (nBufferWidth <= 0 || nBufferHeight <= 0)
		return false;

	if (bNeedAdjust)
	{
		memset(pMaskData, 0, (size_t)width * height);

		int nBufferSize = nBufferWidth * nBufferHeight * GetDataTypeBytes(DataType) * nBandCount;
		//char* pTempBuffer = new char[nBufferSize];
		char* pTempBuffer = (char*)(pDataset->GetMemoryPool()->malloc(nBufferSize));
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


			unsigned char* pDstMaskLine = pMaskData + ((j + ny) * width + nx);
			memset(pDstMaskLine, 255, nBufferWidth);
		}

		//memcpy(pData, pTempBuffer, width * height * GetPixelDataTypeSize(pixelType));
		//delete[] pTempBuffer;
		pDataset->GetMemoryPool()->free((unsigned char*)pTempBuffer);
	}
	else
	{
		pDataset->Read(dAdjustLeft, dAdjustTop, srcWidth, srcHeight, pData, nBufferWidth, nBufferHeight, DataType, nBandCount, bandMap, 3, 3 * nBufferWidth, 1);
	}

	return true;
}

bool TileProcessor::DynamicProject(OGRSpatialReference* ptrVisSRef, Dataset* pDataset, int nBandCount, int bandMap[]
	, const Envelop& envelope, unsigned char* pData, unsigned char* pMaskData, int nWidth, int nHeight, Benchmark& bench_mark)
{
	const Envelop& ptrEnvDataset = pDataset->GetExtent();
	OGRSpatialReference* ptrDSSRef = pDataset->GetSpatialReference();

	RasterResamplingType resampType = NEAREST_NEIGHBOR;

	bool bDynPrjTrans = false;

	bench_mark.TimeTag("DynamicProject");

	// 坐标转换
	if (ptrVisSRef && ptrDSSRef && ptrDSSRef->GetRoot() != nullptr)
	{
// 		if ((ptrDSSRef->GetAttrValue("DATUM") != nullptr && ptrVisSRef->GetAttrValue("DATUM")!= nullptr 
// 			&&  std::string(ptrVisSRef->GetAttrValue("DATUM")).compare(ptrDSSRef->GetAttrValue("DATUM")) == 0) 
// 			|| !ptrVisSRef->IsSame(ptrDSSRef))
// 		{
// 			bDynPrjTrans = true;
// 		}

		if(!ptrVisSRef->IsSame(ptrDSSRef))
			bDynPrjTrans = true;
	}

	bool bDynProjectToGeoCoord = false;
	if (bDynPrjTrans
		&& ptrVisSRef->IsGeographic()
		&& ptrDSSRef->IsProjected())
	{
		bDynProjectToGeoCoord = true;
	}

	Envelop envelopeClone = envelope;
	DataType ePixelType = pDataset->GetDataType();

	bool bTransRes = false;
	if (bDynPrjTrans)
	{
		CoordinateTransformation transformation2(ptrVisSRef, ptrDSSRef);
		bTransRes = transformation2.Transform(envelope, envelopeClone);

		if (bTransRes == false)
		{
			Envelop envelopeClone3 = ptrEnvDataset;
			CoordinateTransformation transformation3(ptrDSSRef, ptrVisSRef);
			bTransRes = transformation3.Transform(ptrEnvDataset, envelopeClone3);

			if (bTransRes && !envelopeClone3.Intersects(envelope))
			{
				return false;
			}
		}

 		double dx1, dy1, dx2, dy2;
 		envelopeClone.QueryCoords(dx1, dy1, dx2, dy2);
 
 		//针对极地坐标做特殊处理
 		if (!bTransRes || std::isnan(dx1) || std::isnan(dy1) || std::isnan(dx2) || std::isnan(dy2))
 		{
			bench_mark.TimeTag("ProcessPerPixel1");
 			return ProcessPerPixel(pDataset, envelope, ptrDSSRef, nWidth, nHeight, nBandCount, bandMap, pData, pMaskData, resampType, bDynProjectToGeoCoord);
 		}
	}

	double dScreenResolutionX = envelope.GetWidth() / (double)nWidth;
	double dScreenResolutionY = envelope.GetHeight() / (double)nHeight;

	int nRasterWid = pDataset->GetRasterXSize();
	int nRasterHei = pDataset->GetRasterYSize();

	envelopeClone.Normalize();
	Envelop ptrEnvInterSect;
	bool bInsecResult = ptrEnvDataset.Intersection(envelopeClone, ptrEnvInterSect);
	if (!bInsecResult)
	{
		//确认一下特殊情况转换是否成功，否则进行逐像素转换
		Envelop ptrEvnTemp = envelope;
		if (IsIntersect(ptrEnvDataset, ptrEvnTemp))
		{
			bench_mark.TimeTag("ProcessPerPixel2");
			return ProcessPerPixel(pDataset, envelope, ptrDSSRef, nWidth, nHeight, nBandCount, bandMap, pData, pMaskData, resampType, bDynProjectToGeoCoord);
		}

		return false;
	}

	double dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
	pDataset->World2Pixel(ptrEnvInterSect.GetXMin(), ptrEnvInterSect.GetYMin(), dx1, dy1);
	pDataset->World2Pixel(ptrEnvInterSect.GetXMin(), ptrEnvInterSect.GetYMax(), dx2, dy2);
	pDataset->World2Pixel(ptrEnvInterSect.GetXMax(), ptrEnvInterSect.GetYMax(), dx3, dy3);
	pDataset->World2Pixel(ptrEnvInterSect.GetXMax(), ptrEnvInterSect.GetYMin(), dx4, dy4);

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

	pDataset->Pixel2World(nRasterWid * 0.3, nRasterHei * 0.4, dxTmp[0], dyTmp[0]);
	pDataset->Pixel2World(nRasterWid * 0.7, nRasterHei * 0.4, dxTmp[1], dyTmp[1]);

	if (bDynPrjTrans)
	{
		CoordinateTransformation coordTrans(ptrDSSRef, ptrVisSRef);
		bool bRes = coordTrans.Transform(2, dxTmp, dyTmp, nullptr);
		if (bRes == false)
		{
			return false;
		}
	}

	double dImgResX = sqrt((dxTmp[0] - dxTmp[1]) * (dxTmp[0] - dxTmp[1]) + (dyTmp[0] - dyTmp[1]) * (dyTmp[0] - dyTmp[1])) / (nRasterWid * 0.4);

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
	MemoryPool* pool = pDataset->GetMemoryPool();
	void* pSrcData = pool->malloc(GetDataTypeBytes(ePixelType) * nDesWid * nDesHei * nBandCount);
	//void* pSrcData = new char[GetDataTypeBytes(ePixelType) * nDesWid * nDesHei * nBandCount];

	//QString pszItem = pDataset->GetMetadataItem("INTERLEAVE", "IMAGE_STRUCTURE");
	//if (pszItem == "BAND" || pszItem == "LINE") // 如果是bsq或者bil格式，则按波段分别读取
	//{
	//	SysDataSource::PixelDataType eType = pDataset->GetRasterBand(0)->GetRasterDataType();
	//	ptrPixelBuffer.Attach(new SysDataSource::PixelBuffer(nDesWid, nDesHei, vecBands, eType));
	//	char* pData = (char*)(ptrPixelBuffer->GetData());
	//	int nBandCount = vecBands.size();

	//	int nTypeSize = SysDataSource::DataSourceUtil::GetTypeSize(eType);
	//	int nPixelSpace = nTypeSize * nBandCount;
	//	int nLineSpace = nPixelSpace * nDesWid;

	//	for (int i = 0; i < nBandCount; i++)
	//	{
	//		SysDataSource::RasterBandPtr ptrRasterBand = pDataset->GetRasterBand(vecBands[i] - 1);
	//		ptrRasterBand->Read(nLeft, nTop, nSrcWid, nSrcHei, pData + i * nTypeSize, nDesWid, nDesHei, eType, nPixelSpace, nLineSpace);

	//		if (ptrTrack && ptrTrack->IsCanced())
	//		{
	//			return false;
	//		}
	//	}
	//}
	//else

	ReadStatistic read_statistic(nDesWid, nDesHei);
	{
		TimeDuration time_duration(read_statistic.read_time_milliseconds);
		pDataset->Read(nLeft, nTop, nSrcWid, nSrcHei, pSrcData, nDesWid, nDesHei, ePixelType, nBandCount, bandMap);
	}
	bench_mark.read_statistics.push_back(read_statistic);

	double dx, dy, dbSrcX, dbSrcY;

	double dImagePosX = 0.0;
	double dImagePosY = 0.0;

	int nTypeSize = GetDataTypeBytes(ePixelType);
	int nDataPixelBytes = nBandCount * nTypeSize;

	//优化显示速度。如果是动态投影，先统一Transform，然后再进行其他操作
	double* pdx = nullptr;
	double* pdy = nullptr;

	double dbHalfCellX = dScreenResolutionX * 0.5;
	double dbHalfCellY = dScreenResolutionY * 0.5;

	bench_mark.TimeTag("bDynPrjTrans");

	if (bDynPrjTrans)
	{
		CoordinateTransformation coordTrans(ptrVisSRef, ptrDSSRef);

		//pdx = new double[nWidth * nHeight];
		//pdy = new double[nWidth * nHeight];

		pdx = (double*)(pool->malloc(nWidth * nHeight * sizeof(double)));
		pdy = (double*)(pool->malloc(nWidth * nHeight * sizeof(double)));

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
// 			delete[] pdx;
// 			delete[] pdy;

			pool->free((unsigned char*)pdx);
			pool->free((unsigned char*)pdy);

			//delete[] (char*)pSrcData;
			pool->free((unsigned char*)pSrcData);
			return false;
		}
	}

	bench_mark.TimeTag("bDynPrjTrans_end");

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
				pDataset->World2Pixel(dbSrcX, dbSrcY, dImagePosX, dImagePosY);
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

					//unsigned char* buffer = new unsigned char[nDataPixelBytes];
					unsigned char* buffer = pool->malloc(nDataPixelBytes);
					pDataset->Read(nImagePosX, nImagePosY, 1, 1, buffer, 1, 1, ePixelType, nBandCount, bandMap);

					unsigned char* pDes = pData + (nWidth * RowIndex + ColIndex) * nDataPixelBytes;
					memcpy(pDes, buffer, nDataPixelBytes);
					//delete[] buffer;
					pool->free(buffer);
				}
				else
				{
					void* pSrc = nullptr;
					unsigned char* pDes = pData + (nWidth * RowIndex + ColIndex) * nDataPixelBytes;

					if (ePixelType < DT_CInt16 && resampType == BILINEAR_INTERPOLATION && nColPos + 1 < nDesWid && nRowPos + 1 < nDesHei)
					{
						//双线性重采样，需要修正0.5个像素。举例：0.8按照双线性，第二个像素占0.8权重，第一个像素占0.2权重
						//实际应该是第一个像素占1.0-(0.8-0.5)=0.7,第二个像素占0.3

						dColPos -= 0.5;
						dRowPos -= 0.5;

						if (dColPos < 0.0)
							dColPos = 0.0;

						if (dRowPos < 0.0)
							dRowPos = 0.0;

						nColPos = dColPos;
						nRowPos = dRowPos;

						double u = dColPos - nColPos;
						double v = dRowPos - nRowPos;

						unsigned char* value1 = (unsigned char*)pSrcData + (nDesWid * nRowPos + nColPos) * nDataPixelBytes;
						unsigned char* value2 = (unsigned char*)pSrcData + (nDesWid * (nRowPos + 1) + nColPos) * nDataPixelBytes;
						unsigned char* value3 = (unsigned char*)pSrcData + (nDesWid * nRowPos + nColPos + 1) * nDataPixelBytes;
						unsigned char* value4 = (unsigned char*)pSrcData + (nDesWid * (nRowPos + 1) + nColPos + 1) * nDataPixelBytes;

						double p1 = (1.0 - u) * (1.0 - v);
						double p2 = (1.0 - u) * v;
						double p3 = u * (1.0 - v);
						double p4 = u * v;

						for (int bandIndex = 0; bandIndex < nBandCount; bandIndex++)
						{
							int offset = bandIndex * nTypeSize;
							switch (ePixelType)
							{
							case DT_Byte:
							{
								unsigned char value = p1 * *(value1 + offset) + p2 * *(value2 + offset)
									+ p3 * *(value3 + offset) + p4 * *(value4 + offset) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_UInt16:
							{
								unsigned short value = p1 * *((unsigned short*)(value1 + offset)) + p2 * *((unsigned short*)(value2 + offset))
									+ p3 * *((unsigned short*)(value3 + offset)) + p4 * *((unsigned short*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_Int16:
							{
								short value = p1 * *((short*)(value1 + offset)) + p2 * *((short*)(value2 + offset))
									+ p3 * *((short*)(value3 + offset)) + p4 * *((short*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_UInt32:
							{
								unsigned int value = p1 * *((unsigned int*)(value1 + offset)) + p2 * *((unsigned int*)(value2 + offset))
									+ p3 * *((unsigned int*)(value3 + offset)) + p4 * *((unsigned int*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_Int32:
							{
								int value = p1 * *((int*)(value1 + offset)) + p2 * *((int*)(value2 + offset))
									+ p3 * *((int*)(value3 + offset)) + p4 * *((int*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_Float32:
							{
								float value = p1 * *((float*)(value1 + offset)) + p2 * *((float*)(value2 + offset))
									+ p3 * *((float*)(value3 + offset)) + p4 * *((float*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							case DT_Float64:
							{
								double value = p1 * *((double*)(value1 + offset)) + p2 * *((double*)(value2 + offset))
									+ p3 * *((double*)(value3 + offset)) + p4 * *((double*)(value4 + offset)) + 0.5;
								memcpy(pDes + offset, &value, nTypeSize);
							}
							break;

							default:
								break;
							}
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

	bench_mark.TimeTag("DynamicProject_end");

	if (bDynPrjTrans)
	{
// 		delete[] pdx;
// 		delete[] pdy;

		pool->free((unsigned char*)pdx);
		pool->free((unsigned char*)pdy);
	}

	//delete[] (char*)pSrcData;
	pool->free((unsigned char*)pSrcData);
	return true;
}

BufferPtr TileProcessor::GetCombinedData(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets
	, const Envelop& env, int tile_width, int tile_height, Format& format, Benchmark& bench_mark)
{
	if (datasets.empty())
		return nullptr;

	MemoryPool* pool_final = nullptr;
	unsigned char* render_buffer_final = nullptr;
	unsigned char* mask_buffer_final = nullptr;

	//Dataset* dataset = datasets.front().first.get();
	//Style* style = datasets.front().second.get();
	//bool bRes = GetTileData(dataset, style, env, tile_width, tile_height, &render_buffer, &mask_buffer);

	int size = tile_width * tile_height;

	bench_mark.TimeTag("GetCombinedData");

	//默认带透明度
	int render_color_count = 4;
	if (format == Format::JPG)
		render_color_count = 3;

	int index_statistic = 0;
	for (auto content : datasets)
	{
		unsigned char* render_buffer = nullptr;
		unsigned char* mask_buffer = nullptr;

		Dataset* dataset = content.first.get();
		Style* style = content.second.get();
		bool bRes = GetTileData(dataset, style, env, tile_width, tile_height, &render_buffer, &mask_buffer, render_color_count, bench_mark);
		
		if (bRes && render_buffer_final == nullptr && mask_buffer_final == nullptr)
		{
			render_buffer_final = render_buffer;
			mask_buffer_final = mask_buffer;

			pool_final = dataset->GetMemoryPool();
		}
		else
		{
			if (bRes && format != Format::JPG)
			{
				for (int i = 0; i < size; i++)
				{
					//此处只是对透明度进行简单处理。如果处理不同的透明度的值的影响，需要进行透明度权重相乘。
					if (mask_buffer[i] != 0)
					{
						int index = i * render_color_count;
						render_buffer_final[index] = render_buffer[index];
						render_buffer_final[index + 1] = render_buffer[index + 1];
						render_buffer_final[index + 2] = render_buffer[index + 2];
						render_buffer_final[index + 3] = render_buffer[index + 3];
					}
				}
			}
			else if (bRes /*&& format == Format::JPG*/)
			{
				for (int i = 0; i < size; i++)
				{
					if (mask_buffer[i] != 0)
					{
						int index = i * render_color_count;
						render_buffer_final[index] = render_buffer[index];
						render_buffer_final[index + 1] = render_buffer[index + 1];
						render_buffer_final[index + 2] = render_buffer[index + 2];
						//render_buffer_final[index + 3] = render_buffer[index + 3];
					}
				}
			}

			if (render_buffer)
			{
				dataset->GetMemoryPool()->free(render_buffer);
			}

			if (mask_buffer)
			{
				dataset->GetMemoryPool()->free(mask_buffer);
			}
		}

		bench_mark.TimeTag("GetCombinedData" + std::to_string(index_statistic ++));
	}

	BufferPtr buffer = nullptr;
	if (render_buffer_final != nullptr)
	{
		switch (format)
		{ 
		case Format::WEBP:
		{
			WebpCompress webpCompress;
			buffer = webpCompress.DoCompress(render_buffer_final, tile_width, tile_height);
		}
			break;

		case Format::PNG:
		{
			PngCompress pngCompress;
			buffer = pngCompress.DoCompress(render_buffer_final, tile_width, tile_height);
		}
			break;
		case Format::JPG:
		{
			JpgCompress jpgCompress;
			buffer = jpgCompress.DoCompress(render_buffer_final, tile_width, tile_height);
		}
		break;
		default:
			break;
		}
	}

	if (render_buffer_final)
	{
		pool_final->free(render_buffer_final);
	}

	if (mask_buffer_final)
	{
		pool_final->free(mask_buffer_final);
	}

	bench_mark.TimeTag("GetCombinedData_End");

	return buffer;
}

bool TileProcessor::GetTileData(Dataset* dataset, Style* style, const Envelop& env, int tile_width, int tile_height
	, unsigned char** buffer_out, unsigned char** mask_buff, int render_color_count, Benchmark& bench_mark)
{
	TiffDataset* tiffDataset = dynamic_cast<TiffDataset*>(dataset);
	if (tiffDataset == nullptr)
		return false;

	//OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(
	//	"GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]");

	int band_count = 3;
	Format format;
	int band_map[4];
	StyleType style_type;
	int epsg_code;

	style->Prepare(tiffDataset);
	style->QueryInfo(band_count, band_map, /*format,*/ style_type, epsg_code);

	//const char* pWKT = "PROJCS[\"WGS_1984_Web_Mercator\",GEOGCS[\"GCS_WGS_1984_Major_Auxiliary_Sphere\",DATUM[\"WGS_1984_Major_Auxiliary_Sphere\",SPHEROID[\"WGS_1984_Major_Auxiliary_Sphere\",6378137.0,0.0]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"False_Easting\",0.0],PARAMETER[\"False_Northing\",0.0],PARAMETER[\"Central_Meridian\",0.0],PARAMETER[\"latitude_of_origin\",0.0],UNIT[\"Meter\",1.0]]";
	//OGRSpatialReference* pDefaultSpatialReference = (OGRSpatialReference*)OSRNewSpatialReference(0/*pWKT*/);
	//pDefaultSpatialReference->importFromEPSG(epsg_code);

	OGRSpatialReference* pDefaultSpatialReference = ResourcePool::GetInstance()->GetSpatialReference(epsg_code);
	int pixel_bytes = GetDataTypeBytes(tiffDataset->GetDataType());

	//bool bSimple = false;
	OGRSpatialReference* poSpatialReference = tiffDataset->GetSpatialReference();
	
	//if (!tiffDataset->IsUseRPC())
	//{
	//	bSimple = poSpatialReference == nullptr ||
	//		//std::string(pDefaultSpatialReference->GetAttrValue("DATUM")).compare(poSpatialReference->GetAttrValue("DATUM")) == 0 ||
	//		poSpatialReference->IsSame(pDefaultSpatialReference);
	//}

	bool bRes = true;
	const size_t nSize = (size_t)tile_width * tile_height * render_color_count * pixel_bytes;

	MemoryPool* pool = dataset->GetMemoryPool();

	*buffer_out = pool->malloc(nSize);;
	unsigned char* buff = *buffer_out;
	unsigned char* mask_buffer = nullptr;
	memset(buff, 255, nSize);

	*mask_buff = pool->malloc((size_t)tile_width * tile_height);
	mask_buffer = *mask_buff;
	memset(mask_buffer, 255, (size_t)tile_width * tile_height);

// 	if (bSimple)
// 	{
// 		bRes = SimpleProject(tiffDataset, band_count, band_map, env, buff, mask_buffer, tile_width, tile_height);
// 	}
// 	else
// 	{
		//env 必须是 pDefaultSpatialReference 的空间参考
		bench_mark.TimeTag("before_DynamicProject");
		bRes = DynamicProject(pDefaultSpatialReference, tiffDataset, band_count, band_map, env, buff, mask_buffer, tile_width, tile_height, bench_mark);
	//}

	if (!bRes)
	{
		//OSRDestroySpatialReference(pDefaultSpatialReference);
		return false;
	}

	style->Apply(buff, mask_buffer, tile_width * tile_height, band_count, band_map, tiffDataset, render_color_count);
	return true;
}

TileProcessor::~TileProcessor()
{
}
