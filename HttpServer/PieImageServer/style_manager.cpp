#include "style_manager.h"
#include "true_color_style.h"
#include "CJsonObject.hpp"
#include "min_max_stretch.h"
#include "histogram_equalize_stretch.h"
#include "standard_deviation_stretch.h"
#include "etcd_storage.h"
#include "percent_min_max_stretch.h"
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/lexical_cast.hpp>

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

std::shared_mutex StyleManager::s_shared_mutex;
std::map<std::string, StylePtr> StyleManager::s_style_map;

std::map<std::string, StylePtr> StyleManager::s_map_style_container;
std::shared_mutex StyleManager::s_shared_mutex_style_container;

//示例：
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "minimumMaximum", "minimum" : [0.0, 0.0, 0.0] , "maximum" : [255.0, 255.0, 255.0] }}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "histogramEqualize", "percent" : 0.0}}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "standardDeviation", "scale" : 2.05}}}

//可以省略一些字段，例如：
//{"style":{"stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}}


bool GetMd5(std::string& str_md5, const char* const buffer, size_t buffer_size)
{
	if (buffer == nullptr)
	{
		return false;
	}
	boost::uuids::detail::md5 boost_md5;
	boost_md5.process_bytes(buffer, buffer_size);
	boost::uuids::detail::md5::digest_type digest;
	boost_md5.get_digest(digest);
	const auto char_digest = reinterpret_cast<const char*>(&digest);
	str_md5.clear();
	boost::algorithm::hex(char_digest, char_digest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(str_md5));
	return true;
}

bool StyleManager::UpdateStyle(const std::string& json_style, std::string& style_key)
{
	StylePtr style = FromJson(json_style);
	if (nullptr == style)
		return false;

	style_key = "";
	EtcdStorage etcd_storage;

	//如果uid为空，则认为是新建的
	if (style->uid_.empty())
	{
		CreateUID(style->uid_);

		style_key = GetStyleKey(style.get());
		std::string new_json = ToJson(style);

		AddOrUpdateStyleMap(style_key, style);
		
		etcd_storage.SetValue(style_key, new_json);
		return true;
	}
	else
	{
		style_key = GetStyleKey(style.get());
		StylePtr oldStyle = GetFromStyleMap(style_key);
		if (oldStyle == nullptr || oldStyle->version_ < style->version_)
		{
			AddOrUpdateStyleMap(style_key, style);

			std::string old_json_style = etcd_storage.GetValue(style_key);
			oldStyle = FromJson(old_json_style);

			if (oldStyle != nullptr && oldStyle->version_ < style->version_)
			{
				etcd_storage.SetValue(style_key, json_style);
				return true;
			}
		}

		return true;
	}

	//std::string style_key;
	//StylePtr style = GetFromStyleMap(style_key);

	//EtcdStorage etcd_storge(Registry::etcd_host_, Registry::etcd_port_);
	//std::string json_style = etcd_storge.GetValue(style_key);
	//if (json_style.empty())
	//{
	//	return false;
	//}
	//
	//AddOrUpdateStyleMap(style_key, style);

	return true;
}

StylePtr StyleManager::GetStyle(const Url& url, const std::string& request_body, DatasetPtr dataset)
{
	std::string file_path = dataset->file_path();
	std::string string_style;
	std::string nodata_value;
	std::string nodata_value_statistic;
	neb::CJsonObject oJson_style;

	url.QueryValue("nodatavalue", nodata_value);
	url.QueryValue("nodatavaluestatistic", nodata_value_statistic);

	bool is_nodata_value_statistic = false;
	if (nodata_value_statistic.compare("true") == 0)
	{
		is_nodata_value_statistic = true;
	}
	else
	{
		nodata_value_statistic = "false";
	}

	//style获取顺序：获取body，如果没有，获取url的style2，如果没有，获取url的style
	if (!request_body.empty())
	{
		neb::CJsonObject oJson(request_body);
		if (oJson.Get("style", oJson_style))
		{
			string_style = oJson_style.ToString();
		}
	}
	else
	{
		url.QueryValue("style2", string_style);
	}
	
	if (!string_style.empty())
	{
		file_path += string_style;
		file_path += nodata_value;
		file_path += nodata_value_statistic;
		std::string md5;
		GetMd5(md5, file_path.c_str(), file_path.size());

		{
			std::shared_lock<std::shared_mutex> lock(s_shared_mutex_style_container);
			std::map<std::string, StylePtr>::iterator itr;

			itr = s_map_style_container.find(md5);
			if (itr != s_map_style_container.end())
			{
				StylePtr style = itr->second->CompletelyClone();
				return style;
			}
		}

		{
			std::unique_lock<std::shared_mutex> lock(s_shared_mutex_style_container);
			std::map<std::string, StylePtr>::iterator itr;

			itr = s_map_style_container.find(md5);
			if (itr != s_map_style_container.end())
			{
				StylePtr style = itr->second->CompletelyClone();
				return style;
			}
			else
			{
				StylePtr style = FromJson(string_style);
				if (style == nullptr)
				{
					LOG(ERROR) << "FromJson(request_body) error";
				}

				if (s_map_style_container.size() > 10000)
				{
					LOG(INFO) << "s_map_style_container cleared";
					s_map_style_container.clear();
				}

				if (!nodata_value.empty())
				{
					double external_no_data_value = boost::lexical_cast<double>(nodata_value);
					style->GetStretch()->SetUseExternalNoDataValue(true);
					style->GetStretch()->SetExternalNoDataValue(external_no_data_value);
					style->GetStretch()->SetStatisticExternalNoDataValue(is_nodata_value_statistic);
				}

				style->Prepare(dataset.get());
				s_map_style_container[md5] = style;
				return style->CompletelyClone();
			}
		}
	}
	else
	{
		std::vector<std::string> tokens;
		std::string style_str = "";
		url.QueryValue("style", style_str);
		Split(style_str, tokens, ":");

		StylePtr style;
		if (tokens.size() == 3)
		{
			StylePtr style1 = GetStyle(tokens[0] + ":" + tokens[1], atoi(tokens[2].c_str()));
			if(style1 != nullptr)
				style = style1->Clone();
		}

		if (style == nullptr)
		{
			style = std::make_shared<Style>();
		}

		return style;
	}
}

std::string StyleManager::GetStyleKey(Style* pStyle)
{
	return StyleType2String(pStyle->kind_) + ':' + pStyle->uid_;
}

StylePtr StyleManager::GetStyle(const std::string& styleKey, size_t version)
{
	StylePtr style = GetFromStyleMap(styleKey);

	if (style == nullptr)
	{
		return nullptr;
	}

	if (style->version_ < version)
	{
		EtcdStorage etcd_storge;
		std::string str_json_style = etcd_storge.GetValue(styleKey);
		style = FromJson(str_json_style);
		if(style == nullptr || style->version_ != version)
		{
			return nullptr;
		}

		AddOrUpdateStyleMap(styleKey, style);
	}

	if (style->version_ != version)
	{
		return nullptr;
	}

	return style;
}

StylePtr StyleManager::FromJson(const std::string& jsonStyle)
{
	if (jsonStyle.empty())
		return nullptr;

	StylePtr style;

	try
	{
		neb::CJsonObject oJson(jsonStyle);
		neb::CJsonObject oJson_style;
		if (oJson.Get("style", oJson_style))
		{
			style = std::make_shared<Style>();
			if (oJson_style.Get("bandCount", style->bandCount_))
			{
				for (int i = 0; i < style->bandCount_; i++)
				{
					oJson_style["bandMap"].Get(i, style->bandMap_[i]);
				}
			}

			oJson_style.Get("version", style->version_);

			style->kind_ = String2StyleType(oJson_style("kind"));
			if (style->kind_ == StyleType::NONE)
			{
				style->kind_ = StyleType::TRUE_COLOR;
			}

			style->format_ = String2Format(oJson_style("format"));

			style->uid_ = oJson_style("uid");

			std::string stretch_kind = oJson_style["stretch"]("kind");
			if (stretch_kind.compare(StretchType2String(StretchType::MINIMUM_MAXIMUM)) == 0)
			{
				double dMin = 0.0;
				double dMax = 0.0;

				auto stretch = std::make_shared<MinMaxStretch>();
				style->stretch_ = stretch;

				for (int i = 0; i < style->bandCount_; i++)
				{
					oJson_style["stretch"]["minimum"].Get(i, stretch->min_value_[i]);
					oJson_style["stretch"]["maximum"].Get(i, stretch->max_value_[i]);
				}
			}
			else if (stretch_kind.compare(StretchType2String(StretchType::PERCENT_MINMAX)) == 0)
			{
				auto stretch = std::make_shared<PercentMinMaxStretch>();
				style->stretch_ = stretch;

				double percent = 1.0;
				oJson_style["stretch"].Get("percent", percent);
				stretch->set_stretch_percent(percent);
			}
			else if (stretch_kind.compare(StretchType2String(StretchType::HISTOGRAMEQUALIZE)) == 0)
			{
				auto stretch = std::make_shared<HistogramEqualizeStretch>();
				style->stretch_ = stretch;

				double percent = 0.0;
				oJson_style["stretch"].Get("percent", percent);
				stretch->set_stretch_percent(percent);
			}
			else if (stretch_kind.compare(StretchType2String(StretchType::STANDARD_DEVIATION)) == 0)
			{
				auto stretch = std::make_shared<StandardDeviationStretch>();
				style->stretch_ = stretch;

				double scale = 2.5;
				oJson_style["stretch"].Get("scale", scale);
				stretch->set_dev_scale(scale);
			}
		}
	}
	catch (...)
	{
		LOG(ERROR) << "style format wrong";
	}

	return style;
}

//目前部署没用到etcd所以暂时不写
std::string StyleManager::ToJson(StylePtr style)
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

StylePtr StyleManager::GetFromStyleMap(const std::string& key)
{	
	std::shared_lock<std::shared_mutex> lock(s_shared_mutex);
	std::map<std::string, StylePtr>::iterator itr;
	
	itr = s_style_map.find(key);
	if (itr != s_style_map.end())
	{
		return itr->second;
	}
	
	return nullptr;
}

bool StyleManager::AddOrUpdateStyleMap(const std::string& key, StylePtr style)
{
	std::unique_lock<std::shared_mutex> lock(s_shared_mutex);
	s_style_map[key] = style;
	return true;
}