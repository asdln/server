#include "histogram_equalize_stretch.h"
#include "dataset.h"
#include "resource_pool.h"

void CaliStretchLinearMinMax_Func(DataType dt, double* ppdHistogram
	, double dParam, double dStep, double dRawMin, double dRawMax, double& dMinValue, double& dMaxValue);

StretchPtr HistogramEqualizeStretch::Clone()
{
	HistogramEqualizeStretchPtr pClone = std::make_shared<HistogramEqualizeStretch>();
	Copy(pClone.get());
	return pClone;
}

void CalFitHistogram(void* pData, DataType datatype, long lTempX, long lTempY, bool bHaveNoData, double dfNoDataValue,
	double& dStep, double& dfMin, double& dfMax, double& dfMean, double& dfStdDev, double* pdHistogram,
	bool bFitMin = false, bool bFitMax = false, double dFitMin = 0.0, double dFitMax = 0.0);

#define HIST_SIZE 256
double* GetCustomHistogram(Dataset* tiff_dataset, int band, double dFitMin, double dFitMax, bool use_external_no_data, double external_no_data_value)
{
	int nSizeX = tiff_dataset->GetRasterXSize();
	int nSizeY = tiff_dataset->GetRasterYSize();

	int have_no_data = 0;
	double dNodataValue = tiff_dataset->GetNoDataValue(band, &have_no_data);
	bool bHaveNoDataValue = have_no_data;

	if (use_external_no_data)
	{
		bHaveNoDataValue = true;
		dNodataValue = external_no_data_value;
	}

	double dXFactor = HIST_SIZE / (float)nSizeX;
	double dYFactor = HIST_SIZE / (float)nSizeY;
	double dFactor = dXFactor < dYFactor ? dXFactor : dYFactor;
	if (dFactor >= 1.0)
	{
		dFactor = 1.0;
	}

	double dReadWidth, dReadHeight;
	dReadWidth = nSizeX;
	dReadHeight = nSizeY;

	long lTempX = (long)(nSizeX * dFactor + 0.5);
	long lTempY = (long)(nSizeY * dFactor + 0.5);

	if (/*m_bCompleteHistogram*/0)
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

	double dfMean = 0.0, dfStdDev = 0.0;
	double      dfMin = 0.0, dfMax = 0.0;
	double      dfNoDataValue;

	void* pData = nullptr;
	double dStep = 0.0;

	double* pdHistogram = nullptr;
	//创建直方图
	pdHistogram = new double[256];
	for (int i = 0; i < 256; i++)
	{
		pdHistogram[i] = 0.0;
	}

	if (bHaveNoDataValue)
	{
		dfNoDataValue = dNodataValue;
	}
	else
	{
		dfNoDataValue = 0.0;
	}

	DataType data_type = tiff_dataset->GetDataType();

	pData = new char[GetDataTypeBytes(data_type) * lSize];

	tiff_dataset->Read(0, 0, dReadWidth, dReadHeight, pData, lTempX, lTempY, data_type, 1, &band);

	CalFitHistogram(pData, data_type, lTempX, lTempY, bHaveNoDataValue, dfNoDataValue,
		dStep, dfMin, dfMax, dfMean, dfStdDev, pdHistogram, true, true, dFitMin, dFitMax);

	delete[] pData;
	return pdHistogram;
}

void HistogramEqualizeStretch::Prepare(int band_count, int* band_map, Dataset* dataset)
{
	if (need_refresh_ == false)
		return;

	//std::lock_guard<std::mutex> guard(mutex_);
	need_refresh_ = false;

	if (stretch_percent_ == 0.0)
	{
		for (int i = 0; i < band_count; i++)
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value_, external_nodata_value_);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);

			min_value_[i] = min;
			max_value_[i] = max;

			HistogramEqualize(histogram->GetHistogram(), i);
		}
	}
	else
	{
		for (int i = 0; i < band_count; i++)
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value_, external_nodata_value_);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);

			double percent_min = 0.0;
			double percent_max = 0.0;

			double dStep = histogram->GetStep();

			CaliStretchLinearMinMax_Func(dataset->GetDataType(), histogram->GetHistogram(), stretch_percent_, dStep, min, max, percent_min, percent_max);

			min_value_[i] = percent_min;
			max_value_[i] = percent_max;

			double* pHistogram = GetCustomHistogram(dataset, band_map[i], percent_min, percent_max, use_external_nodata_value_, external_nodata_value_);

			HistogramEqualize(pHistogram, i);

			if (pHistogram != nullptr)
			{
				delete[] pHistogram;
				pHistogram = nullptr;
			}
		}
	}
}

void HistogramEqualizeStretch::HistogramEqualize(double* pdHistogram, int nBandIndex)
{
	//统计总数
	long double dbTotal = 0;
	for (int i = 0; i < 256; ++i)
	{
		if (pdHistogram[i] > 0)
		{
			dbTotal += pdHistogram[i];
		}
	}

	//计算LUT表
	for (int i = 0; i < 256; ++i)
	{
		lut_[nBandIndex][i] = 0;
	}

	double dbHist = 0;
	for (int i = 0; i < 256; ++i)
	{
		dbHist += pdHistogram[i];
		lut_[nBandIndex][i] = (unsigned char)((dbHist * 255.0) / dbTotal);
	}
}

void HistogramEqualizeStretch::DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset)
{
	MinMaxStretch::DoStretch(data, mask_buffer, size, band_count, band_map, dataset);

	//test code

	//for (int i = 0; i < 256; i++)
	//{
	//	lut_[0][i] = i;
	//	lut_[1][i] = i;
	//	lut_[2][i] = i;
	//	lut_[3][i] = i;
	//}

	//test code end

	unsigned char* render_buffer = (unsigned char*)data;
	for (int j = 0; j < band_count; j++)
	{
		for (int i = 0; i < size; i++)
		{
			render_buffer[i * band_count + j] = lut_[j][render_buffer[i * band_count + j]];
		}
	}

	//memset(render_buffer, 0, size * band_count);
}

void HistogramEqualizeStretch::Copy(HistogramEqualizeStretch* p)
{
	MinMaxStretch::Copy(p);

	p->stretch_percent_ = stretch_percent_;
	for (int j = 0; j < 4; j ++)
	{
		for (int i = 0; i < 256; i++)
		{
			p->lut_[j][i] = lut_[j][i];
		}
	}
}