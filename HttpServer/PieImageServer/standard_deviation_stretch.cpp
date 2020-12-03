#include "standard_deviation_stretch.h"
#include "dataset.h"
#include "resource_pool.h"

StandardDeviationStretch::StandardDeviationStretch()
{
	kind_ = StretchType::STANDARD_DEVIATION;
}

void StandardDeviationStretch::Copy(StandardDeviationStretch* p)
{
	MinMaxStretch::Copy(p);
	p->dev_scale_ = dev_scale_;
}

double StandardDeviationStretch::ChangeDataRange(double value, DataType dt)
{
	double value_temp = 0;
	switch (dt)
	{
	case DataType::DT_Byte:
	{
		if (value < 0.0)
		{
			value_temp = 0;
		}
		else if (value > 255.0)
		{
			value_temp = 255;
		}
		else
		{
			value_temp = value;
		}
	}
	break;
	case DataType::DT_UInt16:
		if (value < 0.0)
		{
			value_temp = 0;
		}
		else if (value > 65535)
		{
			value_temp = 65535;
		}
		else
		{
			value_temp = value;
		}
		break;
	case DataType::DT_Int16:
	{
		if (value < -32768)
		{
			value_temp = -32768;
		}
		else if (value > 32767)
		{
			value_temp = 32767;
		}
		else
		{
			value_temp = value;
		}
	}
	break;
	case DataType::DT_UInt32:
	{
		if (value < 0.0)
		{
			value_temp = 0;
		}
		else if (value > 4294967295.0)
		{
			value_temp = 4294967295;
		}
		else
		{
			value_temp = value;
		}
	}
	break;
	default:
	{
		value_temp = value;
	}
	break;
	}
	return value_temp;
}

void StandardDeviationStretch::Prepare(int band_count, int* band_map, Dataset* dataset)
{
	if (need_refresh_ == false)
		return;

	//std::lock_guard<std::mutex> guard(mutex_);
	need_refresh_ = false;

	for (int i = 0; i < band_count; i++)
	{
		HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value_, external_nodata_value_);
		double min, max, mean, std_dev;
		histogram->QueryStats(min, max, mean, std_dev);

		DataType dt = dataset->GetDataType();
		min_value_[i] = ChangeDataRange(mean - dev_scale_ * std_dev, dt);
		max_value_[i] = ChangeDataRange(mean + dev_scale_ * std_dev, dt);
	}
}

StretchPtr StandardDeviationStretch::Clone()
{
	StandardDeviationStretchPtr pClone = std::make_shared<StandardDeviationStretch>();
	Copy(pClone.get());
	return pClone;
}