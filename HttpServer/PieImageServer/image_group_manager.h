#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <shared_mutex>

class ImageGroupManager
{
public:

	static bool AddImages(const std::string& request_body);

	static bool RemoveImages(const std::string& request_body);

	static bool GetImages(const std::string& request_body, std::string& image_paths_json);

	static bool GetImagesFromLocal(const std::string& group, std::list<std::string>& image_paths);

	static bool SetImages(const std::string& request_body);

	static bool ClearImages(const std::string& request_body);

	static bool GetAllGroup(std::string& groups);

	static bool AddGroups(const std::string& request_body);

	static bool RemoveGroups(const std::string& request_body);

	static bool SetGroupCacheState(const std::string& request_body);

	static bool GetGroupCacheState(const std::string& request_body, std::string& state_json);

	static bool GetGroupCacheState(const std::string& group);

	static bool ClearGroupCache(const std::string& request_body);

	static void StartImageGroupWatch();

protected:

	static void StartImagesWatch();

	static void StartCacheStateWatch();

	static void GetAllGroupsInternal(std::list<std::string>& groups);

	static bool AddImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool RemoveImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool SetImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool GetImagesInternal(const std::string& group, std::string& images_json);

	static bool ClearImagesInternal(const std::string& group);

public:

	static std::string image_group_prefix_;

	static std::shared_mutex s_mutex_;

	static std::unordered_map<std::string, std::list<std::string>> s_user_group_image_map_;

	//Í¼²ã×éµÄ»º´æ×´Ì¬
	static std::unordered_map<std::string, bool> s_group_cache_state_;

	static std::string group_cache_prefix_;

	static std::shared_mutex s_group_cache_mutex_;
};