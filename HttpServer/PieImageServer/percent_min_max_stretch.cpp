#include "percent_min_max_stretch.h"
#include "resource_pool.h"

void CaliStretchLinearMinMax_Func(DataType dt, double* ppdHistogram
	, double dParam, double dStep, double dRawMin, double dRawMax, double& dMinValue, double& dMaxValue)
{
	//统计像素个数
	double dbTotal = 0.0;
	int nUpper = 0;
	for (int i = 0; i < 256; ++i)
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
	int nMin = 0;
	int nMax = 0;

	////判断第一个是否超过dParam%
	for (int i = 0; i < nUpper; ++i)
	{
		if (i == 0 && ppdHistogram[i] / dbTotal > 0.1)
		{
			continue;
		}
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
		if (i == nUpper && ppdHistogram[i] / dbTotal > 0.1)
		{
			continue;
		}
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

	//dStep = (dRawMax - dRawMin) / 255.0;
	if (dt != DT_Byte)
	{
		dMinValue = dRawMin + nMin * dStep;
		dMaxValue = dRawMin + nMax * dStep;
	}
	else
	{
		dMinValue = nMin;
		dMaxValue = nMax;
	}
}

PercentMinMaxStretch::PercentMinMaxStretch()
{
	kind_ = StretchType::PERCENT_MINMAX;
}

void PercentMinMaxStretch::Prepare(int band_count, int* band_map, Dataset* dataset)
{
	if (need_refresh_ == false)
		return;

	std::lock_guard<std::mutex> guard(mutex_);
	need_refresh_ = false;

	for (int i = 0; i < band_count; i++)
	{		
		HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i]);
		double min, max, mean, std_dev;
		histogram->QueryStats(min, max, mean, std_dev);

		CaliStretchLinearMinMax_Func(dataset->GetDataType(), histogram->GetHistogram(), stretch_percent_, histogram->GetStep(), min, max, min_value_[i], max_value_[i]);
	}
}

void PercentMinMaxStretch::Copy(PercentMinMaxStretch*p)
{
	MinMaxStretch::Copy(p);
	p->stretch_percent_ = stretch_percent_;
}

StretchPtr PercentMinMaxStretch::Clone()
{
	PercentMinMaxStretchPtr pClone = std::make_shared<PercentMinMaxStretch>();
	Copy(pClone.get());
	return pClone;
}