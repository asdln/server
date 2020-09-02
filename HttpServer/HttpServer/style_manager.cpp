#include "style_manager.h"
#include "true_color_style.h"
#include "CJsonObject.hpp"
#include "min_max_stretch.h"
#include "etcd_storage.h"
#include "registry.h"

std::shared_mutex StyleManager::s_shared_mutex;
std::map<std::string, StylePtr> StyleManager::s_style_map_;

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

	neb::CJsonObject oJson(jsonStyle);
	auto style = std::make_shared<Style>();

	style->bandCount_ = 4;

	oJson.Get("bandCount", style->bandCount_);
	for (int i = 0; i < style->bandCount_; i ++)
	{
		oJson["bandMap"].Get(i, style->bandMap_[i]);
	}

	oJson.Get("version", style->version_);

	style->uid_ = oJson("uid");

	std::string stretch_kind = oJson["stretch"]("kind");
	if (stretch_kind.compare("minimumMaximum") == 0)
	{
		double dMin = 0.0;
		double dMax = 0.0;

		auto stretch = std::make_shared<MinMaxStretch>();
		style->stretch_ = stretch;

		for (int i = 0; i < style->bandCount_; i++)
		{
			oJson["stretch"]["minimum"].Get(i, stretch->min_value_[i]);
			oJson["stretch"]["maximum"].Get(i, stretch->max_value_[i]);
		}
	}

	return style;
}

std::string StyleManager::ToJson(StylePtr style)
{
	neb::CJsonObject oJson;
	oJson.Add("uid", style->uid_.c_str());
	oJson.Add("version", style->version_);
	oJson.Add("kind", StyleType2String(style->kind_));
	oJson.Add("bandCount", style->bandCount_);
	oJson.AddEmptySubArray("bandMap");

	for (int i = 0; i < style->bandCount_; i ++)
	{
		oJson["bandMap"].Add(style->bandMap_[i]);
	}

	oJson.AddEmptySubObject("stretch");
	if (style->stretch_->kind_ == StretchType::MINIMUM_MAXIMUM)
	{
		oJson["stretch"].Add("kind", "maxMin");
		oJson["stretch"].Add("minimum", 0.0);
		oJson["stretch"].Add("maximum", 255.0);
	}

	return oJson.ToString();
}

StylePtr StyleManager::GetFromStyleMap(const std::string& key)
{	
	std::map<std::string, StylePtr>::iterator itr;
	std::shared_lock<std::shared_mutex> lock(s_shared_mutex);
	itr = s_style_map_.find(key);
	if (itr != s_style_map_.end())
	{
		return itr->second;
	}
	
	return nullptr;
}

bool StyleManager::AddOrUpdateStyleMap(const std::string& key, StylePtr style)
{
	std::unique_lock<std::shared_mutex> lock(s_shared_mutex);
	s_style_map_[key] = style;
	
	return true;
}