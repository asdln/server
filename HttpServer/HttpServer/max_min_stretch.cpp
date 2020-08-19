#include "max_min_stretch.h"


void MaxMinStretch::DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType)
{

}

double MaxMinStretch::min_value()
{
	return min_value_;
}

double MaxMinStretch::max_value()
{
	return max_value_;
}

void MaxMinStretch::set_min_value(double min_value)
{
	min_value_ = min_value;
}

void MaxMinStretch::set_max_value(double max_value)
{
	max_value_ = max_value;
}