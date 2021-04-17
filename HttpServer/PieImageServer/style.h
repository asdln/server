#pragma once

#include <string>
#include <memory>
#include "min_max_stretch.h"
#include "percent_min_max_stretch.h"

#include "CJsonObject.hpp"

class Dataset;

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

class Style;
typedef std::shared_ptr<Style> StylePtr;

class Style
{
	friend class StyleSerielizer;
	friend class StyleManager;

public:

	void Prepare(Dataset* dataset);

	StylePtr Clone();

	StylePtr CompletelyClone();

	StretchPtr GetStretch() { return stretch_; }

	int code() { return srs_epsg_code_; }

	void set_code(int code) { srs_epsg_code_ = code; }

	Format format() { return format_; }

	int band_count() { return bandCount_; }

	void QueryInfo(int band_count, int* band_map, Format& format, StyleType& style_type, int& epsg_code) 
	{ 
		band_count = bandCount_;  
		format = format_; 
		style_type = kind_; 
		epsg_code = srs_epsg_code_; 
		band_map[0] = bandMap_[0];
		band_map[1] = bandMap_[1];
		band_map[2] = bandMap_[2];
		band_map[3] = bandMap_[3];
	}

protected:

	std::string uid_ = "";

	size_t version_ = 0;

	Format format_ = default_format;

	StyleType kind_ = StyleType::TRUE_COLOR;
	int bandMap_[4] = { 1, 2, 3, 4 };
	int bandCount_ = 3;

	//默认 web_mercator
	int srs_epsg_code_ = 3857; //4326 wgs84 

	std::shared_ptr<Stretch> stretch_ = std::make_shared<PercentMinMaxStretch>();
};

class StyleSerielizer
{
public:

	static StylePtr FromJson(const std::string& jsonStyle);

	static StylePtr FromJsonObj(neb::CJsonObject& json_obj);

	static std::string ToJson(StylePtr style);
};


std::string StyleType2String(StyleType style_type);

StyleType String2StyleType(const std::string& str_style);

std::string Format2String(Format format);

Format String2Format(const std::string& format_string);