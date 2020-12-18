#include "style.h"
#include "tiff_dataset.h"
#include "true_color_style.h"
#include "min_max_stretch.h"
#include "histogram_equalize_stretch.h"
#include "standard_deviation_stretch.h"
#include "percent_min_max_stretch.h"
#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

Format default_format = Format::WEBP;
std::string default_string_format = "webp";

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
		string_fromat = default_string_format;
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
		return default_format;
	}
}

StylePtr Style::Clone() 
{
	StylePtr pClone = std::make_shared<Style>();
	pClone->uid_ = uid_;
	pClone->version_ = version_;
	pClone->format_ = format_;
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;
	pClone->stretch_ = stretch_->Clone();
	return pClone;
}

StylePtr Style::CompletelyClone()
{
	StylePtr pClone = std::make_shared<Style>();
	pClone->uid_ = uid_;
	pClone->version_ = version_;
	pClone->format_ = format_;
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;
	pClone->stretch_ = stretch_->Clone();
	pClone->stretch_->need_refresh_ = stretch_->need_refresh_; //完全拷贝，"刷新标记"也拷贝
	return pClone;
}

void Style::Prepare(Dataset* dataset)
{
	TiffDataset* tiffDataset = dynamic_cast<TiffDataset*>(dataset);
	int dataset_band_count = tiffDataset->GetBandCount();
	int nBandCount = bandCount_;
	int bandMap[4] = { bandMap_[0] <= dataset_band_count ? bandMap_[0] : dataset_band_count
		, bandMap_[1] <= dataset_band_count ? bandMap_[1] : dataset_band_count
		, bandMap_[2] <= dataset_band_count ? bandMap_[2] : dataset_band_count
		, bandMap_[3] <= dataset_band_count ? bandMap_[3] : dataset_band_count };


	if (format_ == Format::JPG)
		nBandCount = 3;

	if (nBandCount != 3 && nBandCount != 4)
	{
		nBandCount = 3;
	}

	bandCount_ = nBandCount;
	for (int i = 0; i < bandCount_; i++)
	{
		bandMap_[i] = bandMap[i];
	}

	stretch_->Prepare(bandCount_, bandMap_, dataset);
}


StylePtr StyleSerielizer::FromJsonObj(neb::CJsonObject& json_obj)
{
	StylePtr style;
	style = std::make_shared<Style>();
	if (json_obj.Get("bandCount", style->bandCount_))
	{
		for (int i = 0; i < style->bandCount_; i++)
		{
			json_obj["bandMap"].Get(i, style->bandMap_[i]);
		}
	}

	json_obj.Get("version", style->version_);

	style->kind_ = String2StyleType(json_obj("kind"));
	if (style->kind_ == StyleType::NONE)
	{
		style->kind_ = StyleType::TRUE_COLOR;
	}

	style->format_ = String2Format(json_obj("format"));

	style->uid_ = json_obj("uid");

	double external_nodata_value;
	if (json_obj["stretch"].Get("externalNodataValue", external_nodata_value))
	{
		style->GetStretch()->SetUseExternalNoDataValue(true);
		style->GetStretch()->SetExternalNoDataValue(external_nodata_value);
	}

	std::string stretch_kind = json_obj["stretch"]("kind");
	if (stretch_kind.compare(StretchType2String(StretchType::MINIMUM_MAXIMUM)) == 0)
	{
		double dMin = 0.0;
		double dMax = 0.0;

		auto stretch = std::make_shared<MinMaxStretch>();
		style->stretch_ = stretch;

		for (int i = 0; i < style->bandCount_; i++)
		{
			json_obj["stretch"]["minimum"].Get(i, stretch->min_value_[i]);
			json_obj["stretch"]["maximum"].Get(i, stretch->max_value_[i]);
		}
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::PERCENT_MINMAX)) == 0)
	{
		auto stretch = std::make_shared<PercentMinMaxStretch>();
		style->stretch_ = stretch;

		double percent = 1.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::HISTOGRAMEQUALIZE)) == 0)
	{
		auto stretch = std::make_shared<HistogramEqualizeStretch>();
		style->stretch_ = stretch;

		double percent = 0.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::STANDARD_DEVIATION)) == 0)
	{
		auto stretch = std::make_shared<StandardDeviationStretch>();
		style->stretch_ = stretch;

		double scale = 2.5;
		json_obj["stretch"].Get("scale", scale);
		stretch->set_dev_scale(scale);
	}
	else
	{
		auto stretch = std::make_shared<PercentMinMaxStretch>();
		style->stretch_ = stretch;

		double percent = 0.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}

	return style;
}

StylePtr StyleSerielizer::FromJson(const std::string& jsonStyle)
{
	if (jsonStyle.empty())
		return nullptr;

	StylePtr style;

	try
	{
		neb::CJsonObject oJson(jsonStyle);
		neb::CJsonObject oJson_style;

		//如果不包含"style"，则认为已经提取了字段值
		if (!oJson.Get("style", oJson_style))
		{
			oJson_style = neb::CJsonObject(jsonStyle);
		}

		style = FromJsonObj(oJson_style);
	}
	catch (...)
	{
		LOG(ERROR) << "style format wrong";
	}

	return style;
}

//目前部署没用到etcd所以暂时不写
std::string StyleSerielizer::ToJson(StylePtr style)
{
	neb::CJsonObject oJson;
	//oJson.Add("uid", style->uid_.c_str());
	//oJson.Add("version", style->version_);
	//oJson.Add("kind", StyleType2String(style->kind_));
	//oJson.Add("format", Format2String(style->format_));
	//oJson.Add("bandCount", style->bandCount_);
	//oJson.AddEmptySubArray("bandMap");

	//for (int i = 0; i < style->bandCount_; i ++)
	//{
	//	oJson["bandMap"].Add(style->bandMap_[i]);
	//}

	//oJson.AddEmptySubObject("stretch");
	//if (style->stretch_->kind() == StretchType::MINIMUM_MAXIMUM)
	//{
	//	oJson["stretch"].Add("kind", StretchType2String(StretchType::MINIMUM_MAXIMUM));
	//	oJson["stretch"].Add("minimum", 0.0);
	//	oJson["stretch"].Add("maximum", 255.0);
	//}
	//else if (style->stretch_->kind() == StretchType::PERCENT_MINMAX)
	//{
	//	PercentMinMaxStretchPtr percent_min_max_stretch = std::dynamic_pointer_cast<PercentMinMaxStretch>(style);
	//	oJson["stretch"].Add("kind", StretchType2String(StretchType::PERCENT_MINMAX));
	//	oJson["stretch"].Add("percent", percent_min_max_stretch->stretch_percent());
	//}
	//else if (style->stretch_->kind() == StretchType::HISTOGRAMEQUALIZE)
	//{
	//}

	neb::CJsonObject oJson1;
	oJson1.Add("style", oJson);

	return oJson1.ToString();
}