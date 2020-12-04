#include "histogram_equalize_stretch.h"
#include "dataset.h"
#include "resource_pool.h"
#include <string.h>
#include "application.h"

HistogramEqualizeStretch::HistogramEqualizeStretch()
{
	kind_ = StretchType::PERCENT_MINMAX;

	for (int i = 0; i < 4; i++)
	{
		lut_[i] = new unsigned short[lut_size];
	}
}

HistogramEqualizeStretch::~HistogramEqualizeStretch()
{
	for (int i = 0; i < 4; i++)
	{
		unsigned short* p = lut_[i];
		delete[] p;
		p = nullptr;
	}
}

StretchPtr HistogramEqualizeStretch::Clone()
{
	HistogramEqualizeStretchPtr pClone = std::make_shared<HistogramEqualizeStretch>();
	Copy(pClone.get());
	return pClone;
}

double* GetCustomHistogram(Dataset* tiff_dataset, int band, bool complete_statistic, double dFitMin, double dFitMax, bool use_external_no_data, double external_no_data_value)
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

	int hist_window_size = global_app->StatisticSize();
	double dXFactor = hist_window_size / (float)nSizeX;
	double dYFactor = hist_window_size / (float)nSizeY;
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

	double dfMean = 0.0, dfStdDev = 0.0;
	double      dfMin = 0.0, dfMax = 0.0;
	double      dfNoDataValue;

	void* pData = nullptr;
	double dStep = 0.0;

	double* pdHistogram = nullptr;
	DataType data_type = tiff_dataset->GetDataType();

	int hist_class = CalcClassCount(data_type);
	//创建直方图
	pdHistogram = new double[hist_class];
	for (int i = 0; i < hist_class; i++)
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

	pData = new char[GetDataTypeBytes(data_type) * (size_t)lSize];

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

	bool use_external_nodata_value = use_external_nodata_value_;
	double external_nodata_value = external_nodata_value_;

	if (!nodata_value_statistic)
	{
		use_external_nodata_value = false;
	}

	if (stretch_percent_ == 0.0)
	{
		for (int i = 0; i < band_count; i++)
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value, external_nodata_value);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);
			step_reciprocal_[i] = 1.0 / histogram->GetStep();

			if (dataset->GetDataType() == DT_Byte || dataset->GetDataType() == DT_UInt16)
			{
				hist_min_[i] = 0.0;
			}
			else if (dataset->GetDataType() == DT_Int16)
			{
				hist_min_[i] = -32767.0;
			}
			else
			{
				hist_min_[i] = min;
			}

			HistogramEqualize(histogram->GetHistogram(), histogram->GetClassCount(), i);
		}
	}
	else
	{
		std::map<int, double*> map_temp;
		for (int i = 0; i < band_count; i++)
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value_, external_nodata_value_);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);
			step_reciprocal_[i] = 1.0 / histogram->GetStep();

			if (dataset->GetDataType() == DT_Byte || dataset->GetDataType() == DT_UInt16)
			{
				hist_min_[i] = 0.0;
			}
			else if (dataset->GetDataType() == DT_Int16)
			{
				hist_min_[i] = -32767.0;
			}
			else
			{
				hist_min_[i] = min;
			}

			double percent_min = 0.0;
			double percent_max = 0.0;

			double dStep = histogram->GetStep();

			double* pHistogramCopy = new double[histogram->GetClassCount()];
			memcpy(pHistogramCopy, histogram->GetHistogram(), sizeof(double) * histogram->GetClassCount());

			CalcPercentMinMax_SetHistogram(dataset->GetDataType(), pHistogramCopy, histogram->GetClassCount(), stretch_percent_, dStep, min, percent_min, percent_max);
			HistogramEqualize(pHistogramCopy, histogram->GetClassCount(), i);

			delete[] pHistogramCopy;
			pHistogramCopy = nullptr;
		}

		for (std::map<int, double*>::iterator itr = map_temp.begin(); itr != map_temp.end(); itr ++)
		{
			double* pHistogram = itr->second;
			if (pHistogram != nullptr)
			{
				delete[] pHistogram;
				pHistogram = nullptr;
			}
		}
	}
}

void HistogramEqualizeStretch::HistogramEqualize(const double* pdHistogram, int hist_size, int nBandIndex)
{
	//统计总数
	long double dbTotal = 0;
	for (int i = 0; i < hist_size; ++i)
	{
		if (pdHistogram[i] > 0)
		{
			dbTotal += pdHistogram[i];
		}
	}

	//计算LUT表
	for (int i = 0; i < lut_size; ++i)
	{
		lut_[nBandIndex][i] = 0;
	}

	//double scale = lut_size / (double)hist_class;

	long double dbHist = 0;
	for (int i = 0; i < hist_size; ++i)
	{
		dbHist += pdHistogram[i];
		lut_[nBandIndex][i] = (unsigned short)((dbHist * 255.0) / dbTotal);
	}
}

void HistogramEqualizeStretch::DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset)
{
	int have_no_data[4] = { 0, 0, 0, 0 };
	double no_data_value[4] = { 0.0, 0.0, 0.0, 0.0 };

	if (use_external_nodata_value_)
	{
		for (int i = 0; i < band_count; i++)
		{
			have_no_data[i] = 1;
			no_data_value[i] = external_nodata_value_;
		}
	}
	else
	{
		for (int i = 0; i < band_count; i++)
		{
			no_data_value[i] = dataset->GetNoDataValue(band_map[i], have_no_data + i);
		}
	}

	Prepare(band_count, band_map, dataset);
	DataType dt = dataset->GetDataType();
	int hist_class = CalcClassCount(dt);

	switch (dt)
	{
	case DT_Byte:
	{
		DoStretchImpl_EQL((unsigned char*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_UInt16:
	{
		DoStretchImpl_EQL((unsigned short*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Int16:
	{
		DoStretchImpl_EQL((short*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_UInt32:
	{
		DoStretchImpl_EQL((unsigned int*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Int32:
	{
		DoStretchImpl_EQL((int*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Float32:
	{
		DoStretchImpl_EQL((float*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Float64:
	{
		DoStretchImpl_EQL((double*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CInt16:
	{
		DoStretchImpl2_EQL((short*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CInt32:
	{
		DoStretchImpl2_EQL((int*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat32:
	{
		DoStretchImpl2_EQL((float*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat64:
	{
		DoStretchImpl2_EQL((double*)data, hist_class, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	default:
		break;
	}
}

void HistogramEqualizeStretch::Copy(HistogramEqualizeStretch* p)
{
	Stretch::Copy(p);

	p->stretch_percent_ = stretch_percent_;
	for (int j = 0; j < 4; j ++)
	{
		for (int i = 0; i < lut_size; i++)
		{
			p->lut_[j][i] = lut_[j][i];
		}

		p->step_reciprocal_[j] = step_reciprocal_[j];
		p->hist_min_[j] = hist_min_[j];
	}
}