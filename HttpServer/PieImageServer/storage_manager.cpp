#include "storage_manager.h"

#include "utility.h"
#include "etcd_storage.h"
#include "CJsonObject.hpp"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

std::shared_mutex StorageManager::s_mutex_;

int StorageManager::s_LRU_buffer_size = 1/*2000*/;

std::list<std::string> StorageManager::s_LRU_list;

std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>> StorageManager::s_LRU_data_style_map;


bool StorageManager::AddOrUpdateDataStyle(const std::string& data_style_json, std::string& md5)
{
	Format format;
	//验证data_style_json的格式是否正确
	std::list<std::pair<std::string, std::string>> data_info;
	QueryDataInfo(data_style_json, data_info, format);

	if (data_info.empty())
	{
		LOG(ERROR) << "invalid json: " << data_style_json;
		return false;
	}

	return AddOrUpdateMd5(data_style_json, md5);
}

bool StorageManager::GetDataStyle(const std::string& md5, std::string& data_style_json)
{
	return FindFromMd5(md5, data_style_json);
}

bool StorageManager::AddOrUpdateStyle(const std::string& style_json, std::string& md5)
{
	neb::CJsonObject oJson(style_json);
	neb::CJsonObject oJson_style;

	//简单验证一下格式
	if (!oJson.Get("style", oJson_style) && !oJson.IsArray())
		return false;

	AddOrUpdateMd5(style_json, md5);

	return true;
}

bool StorageManager::GetStyle(const std::string& md5, std::string& style_json)
{
	return FindFromMd5(md5, style_json);
}

bool StorageManager::AddOrUpdateMd5(const std::string& json_str, std::string& md5)
{
	GetMd5(md5, json_str.c_str(), json_str.size());

	//std::shared_lock<std::shared_mutex> lock(s_mutex_);
	std::unique_lock<std::shared_mutex> lock(s_mutex_);

	std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>>::iterator itr = s_LRU_data_style_map.find(md5);
	if (itr == s_LRU_data_style_map.end())
	{
		//如果超出了限制，去掉最少使用的
		if (s_LRU_list.size() >= s_LRU_buffer_size)
		{
			const std::string& least_rencent_used_md5 = s_LRU_list.back();
			s_LRU_data_style_map.erase(least_rencent_used_md5);
			s_LRU_list.pop_back();
		}

		s_LRU_list.push_front(md5);
		s_LRU_data_style_map[md5] = make_pair(json_str, s_LRU_list.begin());
	}
	else
	{
		s_LRU_list.splice(s_LRU_list.begin(), s_LRU_list, s_LRU_data_style_map[md5].second);
		//s_LRU_data_style_map[md5] = make_pair(data_style_json, s_LRU_list.begin());
	}

	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		return etcd_storage.SetValue(md5, json_str, false);
	}
	else
	{
		return true;
	}
}

bool StorageManager::FindFromMd5(const std::string& md5, std::string& data_style_json)
{
	{
		//读锁
		std::shared_lock<std::shared_mutex> lock(s_mutex_);

		std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>>::iterator itr = s_LRU_data_style_map.find(md5);
		if (itr != s_LRU_data_style_map.end() && s_LRU_list.front() == md5)
		{
			data_style_json = itr->second.first;
			return true;
		}
	}

	{
		//写锁
		std::unique_lock<std::shared_mutex> lock(s_mutex_);

		std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>>::iterator itr = s_LRU_data_style_map.find(md5);
		if (itr != s_LRU_data_style_map.end())
		{
			data_style_json = s_LRU_data_style_map[md5].first;
			s_LRU_list.splice(s_LRU_list.begin(), s_LRU_list, s_LRU_data_style_map[md5].second);
			//s_LRU_data_style_map[md5] = make_pair(data_style_json, s_LRU_list.begin());

			return true;
		}
		else
		{
			EtcdStorage etcd_storage;
			if (etcd_storage.IsUseEtcd())
			{
				if (!etcd_storage.GetValue(md5, data_style_json))
				{
					LOG(ERROR) << "can not get value";
					return false;
				}

				if (data_style_json.empty())
				{
					LOG(ERROR) << "can not get value from etcd";
					return false;
				}

				if (s_LRU_list.size() >= s_LRU_buffer_size)
				{
					const std::string& least_rencent_used_md5 = s_LRU_list.back();
					s_LRU_data_style_map.erase(least_rencent_used_md5);
					s_LRU_list.pop_back();
				}

				s_LRU_list.push_front(md5);
				s_LRU_data_style_map[md5] = make_pair(data_style_json, s_LRU_list.begin());
				return true;
			}
		}
	}

	LOG(ERROR) << "ln_debug: FindFromMd5 failed";
	return false;
}