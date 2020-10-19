#ifndef HTTPSERVER_STYLE_H_
#define HTTPSERVER_STYLE_H_

#include <string>
#include <memory>
#include "min_max_stretch.h"
#include "percent_min_max_stretch.h"

enum class StyleType
{
	NONE = 0,
	TRUE_COLOR,
	DEM,
	GRAY
};

enum class Format
{
	JPG = 0,
	PNG,
	WEBP,
	AUTO
};

//json格式样例数据：
/*
{
	"uid":"ece881bb-6020-4b37-93fb-f1b27feda99c",
	"version":2,
	"kind":"trueColor",
	"format":"png",
	"bandMap":[1,2,3],
	"bandCount":3,
	"stretch":
	{
		"kind":"minimumMaximum",
		"minimum":[0,0,0,0],
		"maximum":[255,255,255,255]
	}
}
*/

extern Format default_format;
extern std::string default_string_format;

struct Style
{
	std::string uid_ = "";

	size_t version_ = 0;

	Format format_ = default_format;

	StyleType kind_ = StyleType::TRUE_COLOR;
	int bandMap_[4] = {1, 2, 3, 4};
	int bandCount_ = 3;

	//默认 web_mercator
	int srs_epsg_code_ = 3857; //4326 wgs84 

	std::shared_ptr<Stretch> stretch_ = std::make_shared<PercentMinMaxStretch>();
};

typedef std::shared_ptr<Style> StylePtr;

std::string StyleType2String(StyleType style_type);

StyleType String2StyleType(const std::string& str_style);

std::string Format2String(Format format);

Format String2Format(const std::string& format_string);

#endif //HTTPSERVER_STYLE_H_