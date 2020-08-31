#ifndef HTTPSERVER_ENUMS_H_
#define HTTPSERVER_ENUMS_H_

#include <string>

class Enums
{
};

enum class StyleType
{
	NONE = 0,
	TRUE_COLOR,
	DEM,
	GRAY
};

std::string StyleType2String(StyleType style_type);
StyleType String2StyleType(const std::string& str_style);

enum class StretchType
{
	MINIMUM_MAXIMUM
};

#endif //HTTPSERVER_ENUMS_H_