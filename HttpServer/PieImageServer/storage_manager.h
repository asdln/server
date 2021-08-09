#pragma once

#include <string>
#include <map>
#include <list>
#include <shared_mutex>
#include <unordered_map>

class StorageManager
{
public:

	static bool AddOrUpdateDataStyle(const std::string& data_style_json, std::string& md5);

	static bool GetDataStyle(const std::string& md5, std::string& data_style_json);

	static bool AddOrUpdateStyle(const std::string& style_json, std::string& md5);

	static bool GetStyle(const std::string& md5, std::string& style_json);

protected:

	static bool AddOrUpdateMd5(const std::string& json, std::string& md5);

	static bool FindFromMd5(const std::string& md5, std::string& data_style_json);

protected:

	//static std::map<std::string/*md5*/, std::string/*info的json串*/> s_data_style_map_;

	static std::shared_mutex s_mutex_;


	//LRU 算法，保持s_data_style_map_的大小
	static int s_LRU_buffer_size;
	static std::list<std::string> s_LRU_list;
	static std::unordered_map<std::string/*md5*/, std::pair<std::string/*info的json串*/, std::list<std::string>::iterator/*对应s_LRU_list的迭代器位置*/>> s_LRU_data_style_map;
};