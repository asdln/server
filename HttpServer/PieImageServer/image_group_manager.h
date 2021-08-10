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

	static bool GetImages(const std::string& group, std::list<std::string>& image_paths);

	static bool SetImages(const std::string& request_body);

	static bool ClearImages(const std::string& request_body);

	static bool GetAllGroup(std::string& groups);

	static const std::string& GetPrefix() { return image_group_prefix_; }

protected:

	static void GetAllGroupsInternal(std::list<std::string>& groups);

	static bool AddImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool RemoveImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool SetImagesInternal(const std::string& group, const std::list<std::string>& image_paths);

	static bool GetImagesInternal(const std::string& group, std::string& images_json);

	static bool ClearImagesInternal(const std::string& group);

public:

	static std::unordered_map<std::string, std::list<std::string>> s_etcd_cache_group_image_map_;

	static std::shared_mutex s_etcd_cache_mutex_;

protected:

	static std::string image_group_prefix_;

	static std::shared_mutex s_mutex_;

	static std::unordered_map<std::string, std::list<std::string>> s_user_group_image_map_;
};