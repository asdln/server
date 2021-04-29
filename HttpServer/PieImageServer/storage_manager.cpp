#include "storage_manager.h"

#include "utility.h"
#include "etcd_storage.h"

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
	//��֤data_style_json�ĸ�ʽ�Ƿ���ȷ
	std::list<std::pair<std::string, std::string>> data_info;
	QueryDataInfo(data_style_json, data_info, format);

	if (data_info.empty())
	{
		LOG(ERROR) << "invalid json: " << data_style_json;
		return false;
	}

	GetMd5(md5, data_style_json.c_str(), data_style_json.size());

	//std::shared_lock<std::shared_mutex> lock(s_mutex_);
	std::unique_lock<std::shared_mutex> lock(s_mutex_);

	std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>>::iterator itr = s_LRU_data_style_map.find(md5);
	if (itr == s_LRU_data_style_map.end())
	{
		//������������ƣ�ȥ������ʹ�õ�
		if (s_LRU_list.size() >= s_LRU_buffer_size)
		{
			const std::string& least_rencent_used_md5 = s_LRU_list.back();
			s_LRU_data_style_map.erase(least_rencent_used_md5);
			s_LRU_list.pop_back();
		}

		s_LRU_list.push_front(md5);
		s_LRU_data_style_map[md5] = make_pair(data_style_json, s_LRU_list.begin());
	}
	else
	{
		s_LRU_list.splice(s_LRU_list.begin(), s_LRU_list, s_LRU_data_style_map[md5].second);
		//s_LRU_data_style_map[md5] = make_pair(data_style_json, s_LRU_list.begin());
	}

	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		return etcd_storage.SetValue(md5, data_style_json);
	}
	else
	{
		return true;
	}
}

bool StorageManager::GetDataStyle(const std::string& md5, std::string& data_style_json)
{
	{
		//����
		std::shared_lock<std::shared_mutex> lock(s_mutex_);

		std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>>::iterator itr = s_LRU_data_style_map.find(md5);
		if (itr != s_LRU_data_style_map.end() && s_LRU_list.front() == md5)
		{
			data_style_json = itr->second.first;
			return true;
		}
	}

	{
		//д��
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

	LOG(ERROR) << "unknown error";
	return false;
}