#ifndef HTTPSERVER_STORAGE_MANAGER_H_
#define HTTPSERVER_STORAGE_MANAGER_H_

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

protected:

	//static std::map<std::string/*md5*/, std::string/*info��json��*/> s_data_style_map_;

	static std::shared_mutex s_mutex_;


	//LRU �㷨������s_data_style_map_�Ĵ�С
	static int s_LRU_buffer_size;
	static std::list<std::string> s_LRU_list;
	static std::unordered_map<std::string/*md5*/, std::pair<std::string/*info��json��*/, std::list<std::string>::iterator/*��Ӧs_LRU_list�ĵ�����λ��*/>> s_LRU_data_style_map;
};

#endif //HTTPSERVER_STORAGE_MANAGER_H_