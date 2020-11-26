#include "histogram.h"
#include "utility.h"
#include<algorithm>
#include <cmath>
#include "dataset.h"
#include "string.h"

double ComputerHistogramStepByMinMax(DataType dataType, double dbMin, double dbMax)
{
	//return (dbMax - dbMin) / 255.0;

	double step = 0;
	//计算步长
	switch (dataType)
	{
	case DT_Byte:
	{
		step = 1;
	}
	break;
	case DT_UInt16:
	case DT_Int16:
	case DT_UInt32:
	case DT_Int32:
		// 		{
		// 			int nMin = (int)dbMin;
		// 			int nMax = (int)dbMax;
		// 			int nStep = (int)step;
		// 			if((nMax - nMin) <= 255)
		// 			{
		// 				nStep = 1;
		// 			}
		// 			else
		// 			{
		// 				nStep = (nMax - nMin) / 255;
		// 				nStep = ((nMax - nMin) % 255) > 0 ? (nStep + 1) : nStep;
		// 			}
		// 			step = nStep;
		// 		}
		// 		break;
	case DT_Float32:
	case DT_Float64:
	case DT_CFloat32:
	case DT_CFloat64:
	case DT_CInt16:
	case DT_CInt32:
	{
		step = (dbMax - dbMin) / 255.0;
	}
	break;
	default:
		break;
	}
	return step;
}

void CalFitHistogram(void* pData, DataType datatype, long lTempX, long lTempY, bool bHaveNoData, double dfNoDataValue,
	double& dStep, double& dfMin, double& dfMax, double& dfMean, double& dfStdDev, double* pdHistogram,
	bool bFitMin = false, bool bFitMax = false, double dFitMin = 0.0, double dFitMax = 0.0)
{
	if (pData == nullptr)
		return;

	int bFirstValue = true;
	double dfSum = 0.0, dfSum2 = 0.0;
	long long nSampleCount = 0;
	unsigned char usValue = 0;

	memset(pdHistogram, 0, sizeof(double) * 256);

	//if(!bCheck)
	{
		switch (datatype)
		{
		case DT_Int16:
			bFitMin = true;
			bFitMax = true;
			dFitMin = -32768.0 + 255.0;
			dFitMax = 32767.0 - 255.0;
			break;

		case DT_UInt16:  //hdf数据会存在65535,65534等无效值。此处剔除。
			bFitMax = true;
			dFitMax = 65535.0 - 255.0;
			break;

			//case GDT_UInt32:
			//	bFitMax = true;
			//	dFitMax = 200000;
			//	break;

			//case GDT_Int32:
			//case GDT_Float32:
			//case GDT_Float64:
			//	bFitMin = true;
			//	bFitMax = true;
			//	dFitMin = -200000;
			//	dFitMax = 200000;
			//	break;
		default:
			break;
		}

		for (int iY = 0; iY < lTempY; iY++)
		{
			for (int iX = 0; iX < lTempX; iX++)
			{
				int    iOffset = iX + iY * lTempX;
				double dfValue = 0.0;

				switch (datatype)
				{
				case DT_Byte:
					dfValue = ((unsigned char*)pData)[iOffset];
					break;
				case DT_UInt16:
					dfValue = ((unsigned short*)pData)[iOffset];
					break;
				case DT_Int16:
					dfValue = ((short*)pData)[iOffset];
					break;
				case DT_UInt32:
					dfValue = ((unsigned int*)pData)[iOffset];
					break;
				case DT_Int32:
					dfValue = ((int*)pData)[iOffset];
					break;
				case DT_Float32:
					dfValue = ((float*)pData)[iOffset];
					// 					if (CPLIsNan(dfValue))
					// 						continue;
					break;
				case DT_Float64:
					dfValue = ((double*)pData)[iOffset];
					// 					if (CPLIsNan(dfValue))
					// 						continue;
					break;
				case DT_CInt16:
					//dfValue = ((short *)pData)[iOffset * 2];
				{
					double d1 = ((short*)pData)[iOffset * 2] * ((short*)pData)[iOffset * 2];
					double d2 = ((short*)pData)[iOffset * 2 + 1] * ((short*)pData)[iOffset * 2 + 1];
					dfValue = sqrt(d1 + d2);
				}
				break;
				case DT_CInt32:
					//dfValue = ((int *)pData)[iOffset * 2];
				{
					double d1 = ((int*)pData)[iOffset * 2] * ((int*)pData)[iOffset * 2];
					double d2 = ((int*)pData)[iOffset * 2 + 1] * ((int*)pData)[iOffset * 2 + 1];
					dfValue = sqrt(d1 + d2);
				}
				break;
				case DT_CFloat32:
					//dfValue = ((float *)pData)[iOffset * 2];

				{
					double d1 = ((float*)pData)[iOffset * 2];
					double d2 = ((float*)pData)[iOffset * 2 + 1];
					dfValue = sqrt(d1 * d1 + d2 * d2);
				}
				// 					if (CPLIsNan(dfValue))
				// 						continue;
				break;
				case DT_CFloat64:
					//dfValue = ((double *)pData)[iOffset * 2];
					// 					if (CPLIsNan(dfValue))
					// 						continue;
				{
					double d1 = ((double*)pData)[iOffset * 2];
					double d2 = ((double*)pData)[iOffset * 2 + 1];
					dfValue = sqrt(d1 * d1 + d2 * d2);
				}
				break;
				default:
					//					CPLAssert(FALSE);
					break;
				}

				if ((bHaveNoData && dfValue == dfNoDataValue) || std::isnan(dfValue) || !std::isfinite(dfValue))
					continue;
				if (bFitMin && dfValue < dFitMin)
					continue;
				if (bFitMax && dfValue > dFitMax)
					continue;

				if (bFirstValue)
				{
					dfMin = dfMax = dfValue;
					bFirstValue = false;
				}
				else
				{
					dfMin = std::min(dfMin, dfValue);
					dfMax = std::max(dfMax, dfValue);
				}

				dfSum += dfValue;
				dfSum2 += dfValue * dfValue;

				nSampleCount++;
				if (datatype == DT_Byte)
				{
                    usValue = (unsigned char)(dfValue);
					pdHistogram[usValue]++;
				}
			}
		}
		dfMean = dfSum / nSampleCount;
		dfStdDev = sqrt((dfSum2 / nSampleCount) - (dfMean * dfMean));
		dStep = ComputerHistogramStepByMinMax(datatype, dfMin, dfMax);
	}

	// 		if(bFitMin)
	// 		{
	// 			dfMin = dFitMin;
	// 		}
	// 		if(bFitMax)
	// 		{
	// 			dfMax = dFitMax;
	// 		}

	if (datatype == DT_Byte)
	{
		return;
	}
	for (int iY = 0; iY < lTempY; ++iY)
	{
		for (int iX = 0; iX < lTempX; iX++)
		{
			int    iOffset = iX + iY * lTempX;
			double dfValue = 0.0;

			switch (datatype)
			{
			case DT_Byte:
				dfValue = ((unsigned char*)pData)[iOffset];
				break;
			case DT_UInt16:
				dfValue = ((unsigned short*)pData)[iOffset];
				break;
			case DT_Int16:
				dfValue = ((short*)pData)[iOffset];
				break;
			case DT_UInt32:
				dfValue = ((unsigned int*)pData)[iOffset];
				break;
			case DT_Int32:
				dfValue = ((int*)pData)[iOffset];
				break;
			case DT_Float32:
				dfValue = ((float*)pData)[iOffset];
				// 				if (CPLIsNan(dfValue))
				// 					continue;
				break;
			case DT_Float64:
				dfValue = ((double*)pData)[iOffset];
				// 				if (CPLIsNan(dfValue))
				// 					continue;
				break;
			case DT_CInt16:
				dfValue = ((short*)pData)[iOffset * 2];
				break;
			case DT_CInt32:
				dfValue = ((int*)pData)[iOffset * 2];
				break;
			case DT_CFloat32:
				//dfValue = ((float *)pData)[iOffset * 2];
			{
				double d1 = ((float*)pData)[iOffset * 2];
				double d2 = ((float*)pData)[iOffset * 2 + 1];
				dfValue = sqrt(d1 * d1 + d2 * d2);
			}
		// 				if (CPLIsNan(dfValue))
		// 					continue;
			break;
			case DT_CFloat64:
				//dfValue = ((double *)pData)[iOffset * 2];
				// 				if (CPLIsNan(dfValue))
				// 					continue;
			{
				double d1 = ((double*)pData)[iOffset * 2];
				double d2 = ((double*)pData)[iOffset * 2 + 1];
				dfValue = sqrt(d1 * d1 + d2 * d2);
			}
			break;
			default:
				//				CPLAssert(FALSE);
				break;
			}

			if ((bHaveNoData && dfValue == dfNoDataValue) || std::isnan(dfValue) || !std::isfinite(dfValue))
				continue;
			if (bFitMin && dfValue < dFitMin)
				continue;
			if (bFitMax && dfValue > dFitMax)
				continue;

			if (dfValue <= dfMin)
			{
				dfValue = dfMin;
			}
			if (dfValue >= dfMax)
			{
				dfValue = dfMax;
			}
			if (dStep != 0)// && datatype != GDT_Byte
			{
				usValue = (unsigned char)((dfValue - dfMin) / dStep);

			}
			else
			{
				usValue = (unsigned char)(dfValue);
			}
			//归一化(对于BYTE类型)
			pdHistogram[usValue]++;
		}
	}
}

const int hist_size = 256;

HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic, bool use_external_no_data, double external_no_data_value)
{
	int m_nSizeX = dataset->GetRasterXSize();
	int m_nSizeY = dataset->GetRasterYSize();

	double dXFactor = hist_size / (float)m_nSizeX;
	double dYFactor = hist_size / (float)m_nSizeY;
	double dFactor = dXFactor < dYFactor ? dXFactor : dYFactor;
	if (dFactor >= 1.0)
	{
		dFactor = 1.0;
	}

	double dReadWidth, dReadHeight;
	dReadWidth = m_nSizeX;
	dReadHeight = m_nSizeY;

	long lTempX = (long)(m_nSizeX * dFactor + 0.5);
	long lTempY = (long)(m_nSizeY * dFactor + 0.5);

	if (complete_statistic)
	{
		lTempX = dReadWidth;
		lTempY = dReadHeight;
	}

	if (lTempX <= 0)
		lTempX = 1;

	if (lTempY <= 0)
		lTempY = 1;

	long lSize = lTempX * lTempY;
	long lFittingLevel = 0;

	double dfMean = 0.0, std_dev = 0.0;
	double      dfMin = 0.0, dfMax = 0.0;

	void* pData = nullptr;
	double dStep = 0.0;

	double* pdHistogram = nullptr;
	//创建直方图
	pdHistogram = new double [hist_size];
	memset(pdHistogram, 0, sizeof(double) * hist_size);

	int have_no_data = 0;
	double no_data_value = dataset->GetNoDataValue(band, &have_no_data);
	DataType data_type = dataset->GetDataType();

	if (use_external_no_data)
	{
		have_no_data = true;
		no_data_value = external_no_data_value;
	}

	pData = new char[GetDataTypeBytes(data_type) * lSize];

	dataset->Read(0, 0, dReadWidth, dReadHeight, pData, lTempX, lTempY, data_type, 1, &band);
	CalFitHistogram(pData, data_type, lTempX, lTempY, have_no_data, no_data_value,
		dStep, dfMin, dfMax, dfMean, std_dev, pdHistogram);
	
	HistogramPtr histogram = std::make_shared<Histogram>();
	histogram->SetStats(dfMin, dfMax, dfMean, std_dev);
	if (pdHistogram != nullptr)
	{
		histogram->SetHistogram(pdHistogram);
	}

	histogram->SetStep(dStep);
	delete[] pData;

	return histogram;
}

Histogram::Histogram(void)
{

}

Histogram::~Histogram(void)
{
	if (nullptr != histogram_)
	{
		delete[] histogram_;
		histogram_ = nullptr;
	}
}

void Histogram::SetStats(double min, double max, double mean, double std_dev)
{
	minimum_ = min;
	maximum_ = max;
	mean_ = mean;
	std_dev_ = std_dev;
}

void Histogram::QueryStats(double& min, double& max, double& mean, double& std_dev)
{
	min = minimum_;
	max = maximum_;
	mean = mean_;
	std_dev = std_dev_;
}

void Histogram::SetHistogram(double* histogram)
{
	if (nullptr == histogram)
	{
		return;
	}

	histogram_ = histogram;
}

double* Histogram::GetHistogram()
{
	return histogram_;
}
