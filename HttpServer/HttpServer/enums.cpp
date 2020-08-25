#include "enums.h"


std::string StyleType2String(StyleType style_type)
{
	std::string str_style = "";

	switch (style_type)
	{
	case StyleType::TRUE_COLOR:
		str_style = "trueColor";
		break;
	case StyleType::DEM:
		str_style = "dem";
		break;
	case StyleType::GRAY:
		str_style = "gray";
		break;
	default:
		str_style = "none";
		break;
	}

	return str_style;
}

StyleType String2StyleType(const std::string& str_style)
{
	if (str_style.compare("trueColor") == 0)
	{
		return StyleType::TRUE_COLOR;
	}
	else if (str_style.compare("dem") == 0)
	{
		return StyleType::DEM;
	}
	else if (str_style.compare("gray") == 0)
	{
		return StyleType::GRAY;
	}

	return StyleType::NONE;
}