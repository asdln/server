#include "style.h"


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

std::string Format2String(Format format)
{
	std::string string_fromat = "";
	switch (format)
	{
	case Format::JPG:
		string_fromat = "jpg";
		break;
	case Format::PNG:
		string_fromat = "png";
		break;
	case Format::WEBP:
		string_fromat = "webp";
		break;
	case Format::AUTO:
		string_fromat = "auto";
		break;
	default:
		string_fromat = "png";
		break;
	}

	return string_fromat;
}

Format String2Format(const std::string& format_string)
{
	if (format_string.compare("jpg") == 0)
	{
		return Format::JPG;
	}
	else if (format_string.compare("png") == 0)
	{
		return Format::PNG;
	}
	else if (format_string.compare("webp") == 0)
	{
		return Format::WEBP;
	}
	else if (format_string.compare("auto") == 0)
	{
		return Format::AUTO;
	}
	else
	{
		return Format::PNG;
	}
}