#pragma once

#include <string>
#include <memory>
#include "min_max_stretch.h"
#include "percent_min_max_stretch.h"

#include "CJsonObject.hpp"

class Dataset;


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

	void Apply(void* data, unsigned char* mask_buffer, int size
		, int band_count, int* band_map, Dataset* dataset, int render_color_count);

	StylePtr Clone();

	StylePtr CompletelyClone();

	void set_stretch(std::shared_ptr<Stretch> stretch) { stretch_ = stretch; }

	StretchPtr get_stretch() { return stretch_; }

	int code() { return srs_epsg_code_; }

	void set_code(int code) { srs_epsg_code_ = code; }

	//Format format() { return format_; }

	void set_band_map(int* band_map, int band_count);

	void set_band_count(int band_count) { bandCount_ = band_count; }

	int band_count() { return bandCount_; }

	void set_kind(StyleType kind) { kind_ = kind; }

	void init_lut();

	StyleType get_kind() { return kind_; }

	void QueryInfo(int& band_count, int* band_map, /*Format& format, */StyleType& style_type, int& epsg_code) 
	{ 
		band_count = bandCount_;  
		//format = format_; 
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

	StyleType kind_ = StyleType::TRUE_COLOR;
	int bandMap_[4] = { 1, 2, 3, 4 };
	int bandCount_ = 3;

	//默认 web_mercator
	int srs_epsg_code_ = 3857; //4326 wgs84 

	std::shared_ptr<Stretch> stretch_ = std::make_shared<PercentMinMaxStretch>();

	unsigned char lut_[256][4];
};

class StyleSerielizer
{
public:

	static StylePtr FromJson(const std::string& jsonStyle);

	static StylePtr FromJsonObj(neb::CJsonObject& json_obj);

	//static std::string ToJson(StylePtr style);
};


std::string StyleType2String(StyleType style_type);

StyleType String2StyleType(const std::string& str_style);

std::string Format2String(Format format);

Format String2Format(const std::string& format_string);