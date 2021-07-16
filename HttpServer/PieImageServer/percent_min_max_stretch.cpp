#include "percent_min_max_stretch.h"
#include "resource_pool.h"

PercentMinMaxStretch::PercentMinMaxStretch()
{
	kind_ = StretchType::PERCENT_MINMAX;
}

void PercentMinMaxStretch::Prepare(int band_count, int* band_map, Dataset* dataset)
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

	for (int i = 0; i < band_count; i++)
	{		
		if (stretch_percent_ == 0.0 && dataset->GetDataType() == DT_Byte)
		{
			min_value_[i] = 0;
			max_value_[i] = 255;
		}
		else
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value, external_nodata_value);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);

			if (stretch_percent_ == 0.0)
			{
				min_value_[i] = min;
				max_value_[i] = max;
			}
			else
			{
				int nMin, nMax;
				CalcPercentMinMax(dataset->GetDataType(), histogram->GetHistogram(), histogram->GetClassCount(), stretch_percent_, histogram->GetStep(), min, min_value_[i], max_value_[i], nMin, nMax);
			}

		}
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