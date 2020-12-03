#include "histogram.h"
#include "utility.h"
#include<algorithm>
#include <cmath>
#include "dataset.h"
#include "string.h"

bool g_complete_statistic = false;

int CalcClassCount(DataType dt)
{
	if (dt == DT_Byte)
		return 256;
	else
		return 65536;
}

void CalcPercentMinMax(DataType dt, const double* ppdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue, int& nMin, int& nMax)
{
	//统计像素个数
	double dbTotal = 0.0;
	int nUpper = 0;
	for (int i = 0; i < hist_class; ++i)
	{
		if (ppdHistogram[i] > 0)
		{
			dbTotal += ppdHistogram[i];
			nUpper = i;
		}
	}
	//统计dParam以下的值
	double dbSum = 0;
	double dbSum_Next = 0;
	nMin = 0;
	nMax = 0;

	////判断第一个是否超过dParam%
	for (int i = 0; i < nUpper; ++i)
	{
		//if (i == 0 && ppdHistogram[i] / dbTotal > 0.1)
		//{
		//	continue;
		//}
		dbSum += ppdHistogram[i];
		dbSum_Next = dbSum + ppdHistogram[i + 1];
		if ((dbSum_Next / dbTotal) >= (dParam / 100.0))// && ppdHistogram[i][1]/dbTotal > 1e-4 && i != 0
		{
			nMin = i;
			if (fabs((dbSum / dbTotal) - dParam / 100.0) - fabs((dbSum_Next / dbTotal) - dParam / 100.0) > 1e-10)
			{
				nMin++;
			}
			break;
		}
	}
	dbSum = dbTotal;
	//统计dParam%以上的值
	for (int i = nUpper; i > 0; --i)
	{
		//if (i == nUpper && ppdHistogram[i] / dbTotal > 0.1)
		//{
		//	continue;
		//}
		dbSum -= ppdHistogram[i];
		dbSum_Next = dbSum - ppdHistogram[i - 1];

		if ((dbSum / dbTotal) <= ((100 - dParam) / 100.0))// && ppdHistogram[i][1]/dbTotal > 1e-4 && i != nUpper
		{
			nMax = i;
			if (fabs((dbSum / dbTotal) - ((100 - dParam) / 100.0)) - fabs((dbSum_Next / dbTotal) - ((100 - dParam) / 100.0)) > 1e-10)
			{
				nMax--;
			}
			break;
		}
	}

	//特殊情况特殊处理
	if (nMin == 0 && nMax == 0)
	{
		nMax = 1;
	}

	//之前step默认设了1.0，所以要特殊处理
	if (dt == DT_Byte || dt == DT_UInt16)
	{
		dMinValue = nMin;
		dMaxValue = nMax;
	}
	else if (dt == DT_Int16)
	{
		dMinValue = -32767.0;
		dMaxValue = 32768.0;
	}
	else
	{
		dMinValue = dRawMin + nMin * dStep;
		dMaxValue = dRawMin + nMax * dStep;
	}
}

void CalcPercentMinMax_SetHistogram(DataType dt, double* pdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue)
{
	int nMin = 0;
	int nMax = 0;
	CalcPercentMinMax(dt, pdHistogram, hist_class, dParam, dStep, dRawMin, dMinValue, dMaxValue, nMin, nMax);

	for (int i = 0; i <= nMin; i++)
	{
		pdHistogram[i] = 0.0;
	}

	for (int i = nMax; i < hist_class; i ++)
	{
		pdHistogram[i] = 0.0;
	}
}

double ComputerHistogramStepByMinMax(DataType dataType, double dbMin, double dbMax, int hist_class)
{
	double step = 0;
	//计算步长
	switch (dataType)
	{
	case DT_Byte:
	case DT_UInt16:
	case DT_Int16:
	{
		step = 1.0;
	}
	break;

	case DT_UInt32:
	case DT_Int32:
	case DT_Float32:
	case DT_Float64:
	case DT_CFloat32:
	case DT_CFloat64:
	case DT_CInt16:
	case DT_CInt32:
	{
		step = (dbMax - dbMin) / (double)(hist_class - 1);
	}
	break;
	default:
		break;
	}
	return step;
}

void CalFitHistogram(void* pData, DataType datatype, long lTempX, long lTempY, 
	bool bHaveNoData, double dfNoDataValue, double& dStep, double& dfMin, double& dfMax, 
	double& dfMean, double& dfStdDev, double* pdHistogram, int hist_class,
	bool bFitMin, bool bFitMax, double dFitMin, double dFitMax)
{
	if (pData == nullptr)
		return;

	int bFirstValue = true;
	long double dfSum = 0.0, dfSum2 = 0.0;
	long long nSampleCount = 0;

	memset(pdHistogram, 0, sizeof(double) * hist_class);

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
				double d1 = ((short*)pData)[iOffset * 2] * (double)((short*)pData)[iOffset * 2];
				double d2 = ((short*)pData)[iOffset * 2 + 1] * (double)((short*)pData)[iOffset * 2 + 1];
				dfValue = sqrt(d1 + d2);
			}
			break;
			case DT_CInt32:
				//dfValue = ((int *)pData)[iOffset * 2];
			{
				double d1 = ((int*)pData)[iOffset * 2] * (double)((int*)pData)[iOffset * 2];
				double d2 = ((int*)pData)[iOffset * 2 + 1] * (double)((int*)pData)[iOffset * 2 + 1];
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
				unsigned char usValue = dfValue;
				pdHistogram[usValue]++;
			}
			else if (datatype == DT_UInt16)
			{
				unsigned short usValue = dfValue;
				pdHistogram[usValue]++;
			}
			else if (datatype == DT_Int16)
			{
				unsigned short usValue = dfValue + 32767;
				pdHistogram[usValue]++;
			}
		}
	}

	dfMean = dfSum / nSampleCount;
	dfStdDev = sqrt((dfSum2 / nSampleCount) - (dfMean * dfMean));
	dStep = ComputerHistogramStepByMinMax(datatype, dfMin, dfMax, hist_class);

	if (datatype == DT_Byte || datatype == DT_UInt16 || datatype == DT_Int16)
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
				unsigned short usValue = (unsigned short)((dfValue - dfMin) / dStep);
				pdHistogram[usValue]++;
			}
			else
			{
				unsigned short usValue = (unsigned short)(dfValue);
				pdHistogram[usValue]++;
			}
		}
	}
}

HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic, 
	bool use_external_no_data, double external_no_data_value)
{
	int m_nSizeX = dataset->GetRasterXSize();
	int m_nSizeY = dataset->GetRasterYSize();

	double dXFactor = hist_window_size / (float)m_nSizeX;
	double dYFactor = hist_window_size / (float)m_nSizeY;
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

	double dfMean = 0.0, std_dev = 0.0;
	double dfMin = 0.0, dfMax = 0.0;

	void* pData = nullptr;
	double dStep = 1.0;

	int have_no_data = 0;
	double no_data_value = dataset->GetNoDataValue(band, &have_no_data);
	DataType data_type = dataset->GetDataType();

	int hist_class = CalcClassCount(data_type);
	double* pdHistogram = nullptr;
	//创建直方图
	pdHistogram = new double [hist_class];
	memset(pdHistogram, 0, sizeof(double) * hist_class);

	if (use_external_no_data)
	{
		have_no_data = true;
		no_data_value = external_no_data_value;
	}

	pData = new char[GetDataTypeBytes(data_type) * (long)lSize];

	dataset->Read(0, 0, dReadWidth, dReadHeight, pData, lTempX, lTempY, data_type, 1, &band);
	CalFitHistogram(pData, data_type, lTempX, lTempY, have_no_data, no_data_value,
		dStep, dfMin, dfMax, dfMean, std_dev, pdHistogram, hist_class);
	
	HistogramPtr histogram = std::make_shared<Histogram>();
	histogram->SetStats(dfMin, dfMax, dfMean, std_dev);
	if (pdHistogram != nullptr)
	{
		histogram->SetHistogram(pdHistogram);
	}

	histogram->SetStep(dStep);
	histogram->SetClassCount(hist_class);
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

const double* Histogram::GetHistogram()
{
	return histogram_;
}
