#include "stretch.h"


std::string StretchType2String(StretchType type)
{
	std::string type_string;
	switch (type)
	{
	case StretchType::MINIMUM_MAXIMUM:
		type_string = "minimumMaximum";
		break;
	case StretchType::PERCENT_MINMAX:
		type_string = "percentMinimumMaximum";
		break;
	case StretchType::HISTOGRAMEQUALIZE:
		type_string = "histogramEqualize";
		break;
	case StretchType::STANDARD_DEVIATION:
		type_string = "standardDeviation";
		break;
	default:
		type_string = "minimumMaximum";
		break;
	}

	return type_string;
}

StretchType String2StretchType(std::string type_string)
{
	StretchType type = StretchType::MINIMUM_MAXIMUM;
	if (type_string.compare("minimumMaximum") == 0)
	{
		type = StretchType::MINIMUM_MAXIMUM;
	}
	else if (type_string.compare("percentMinimumMaximum") == 0)
	{
		type = StretchType::PERCENT_MINMAX;
	}
	else if (type_string.compare("histogramEqualize") == 0)
	{
		type = StretchType::HISTOGRAMEQUALIZE;
	}
	else if (type_string.compare("standardDeviation") == 0)
	{
		type = StretchType::STANDARD_DEVIATION;
	}
	return type;
}

void Stretch::SetUseExternalNoDataValue(bool useExternal)
{
	use_external_nodata_value_ = useExternal;
}

bool Stretch::GetUseExternalNoDataValue()
{
	return use_external_nodata_value_;
}

void Stretch::SetExternalNoDataValue(double external_value)
{
	external_nodata_value_ = external_value;
}

double Stretch::GetExternalNoDataValue()
{
	return external_nodata_value_;
}

void Stretch::Copy(Stretch* p)
{
	p->need_refresh_ = /*need_refresh_*/true; //克隆对象，默认需要更新。因为不知道关联到哪个数据集
	p->kind_ = kind_;
	p->use_external_nodata_value_ = use_external_nodata_value_;
	p->external_nodata_value_ = external_nodata_value_;
}