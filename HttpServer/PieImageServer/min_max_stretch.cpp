#include "min_max_stretch.h"
#include "dataset.h"
#include "resource_pool.h"

MinMaxStretch::MinMaxStretch()
{
	kind_ = StretchType::MINIMUM_MAXIMUM;
}

StretchPtr MinMaxStretch::Clone()
{
	MinMaxStretchPtr pClone = std::make_shared<MinMaxStretch>();
	Copy(pClone.get());
	return pClone;
}

void MinMaxStretch::Prepare(int band_count, int* band_map, Dataset* dataset)
{
	if (need_refresh_ == false)
		return;

	//std::lock_guard<std::mutex> guard(mutex_);
	need_refresh_ = false;

	for (int i = 0; i < band_count; i++)
	{
		if (min_value_[i] == 0.0 && max_value_[i] == 0.0)
		{
			HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(dataset, band_map[i], use_external_nodata_value_, external_nodata_value_);
			double min, max, mean, std_dev;
			histogram->QueryStats(min, max, mean, std_dev);
			min_value_[i] = min;
			max_value_[i] = max;
		}
	}
}

void MinMaxStretch::DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset)
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

	switch (dataset->GetDataType())
	{
	case DT_Byte:
	{
		DoStretchImpl((unsigned char*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_UInt16:
	{
		DoStretchImpl((unsigned short*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Int16:
	{
		DoStretchImpl((short*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_UInt32:
	{
		DoStretchImpl((unsigned int*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Int32:
	{
		DoStretchImpl((int*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Float32:
	{
		DoStretchImpl((float*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_Float64:
	{
		DoStretchImpl((double*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CInt16:
	{
		DoStretchImpl2((short*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CInt32:
	{
		DoStretchImpl2((int*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat32:
	{
		DoStretchImpl2((float*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat64:
	{
		DoStretchImpl2((double*)data, mask_buffer, size, band_count, no_data_value, have_no_data);
		break;
	}
	default:
		break;
	}
}

void MinMaxStretch::Copy(MinMaxStretch* p)
{
	Stretch::Copy(p);

	for (int i = 0; i < 4; i++)
	{
		p->min_value_[i] = min_value_[i];
		p->max_value_[i] = max_value_[i];
	}
}