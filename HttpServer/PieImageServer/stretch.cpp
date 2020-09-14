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

	return type;
}