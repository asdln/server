#include "image_group_manager.h"
#include "etcd_storage.h"
#include "CJsonObject.hpp"

std::shared_mutex ImageGroupManager::s_mutex_;

std::unordered_map<std::string, std::set<std::string>> ImageGroupManager::s_user_group_image_map_;

bool ImageGroupManager::AddImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string user;
	std::string group;
	std::set<std::string> image_paths;
	if (oJson.Get("user", user) && oJson.Get("group", group))
	{
		neb::CJsonObject images_obj;
		if (oJson.Get("images", images_obj))
		{
			if (images_obj.IsArray())
			{
				int array_size = images_obj.GetArraySize();
				for (int i = 0; i < array_size; i ++)
				{
					std::string path;
					images_obj.Get(i, path);
					image_paths.emplace(path);
				}

				return AddImages(user, group, image_paths);
			}
		}
	}

	return false;
}

bool ImageGroupManager::GetImages(const std::string& request_body, std::string& image_paths_json)
{
	neb::CJsonObject oJson(request_body);

	std::string user;
	std::string group;
	
	if (oJson.Get("user", user) && oJson.Get("group", group))
	{
		return GetImages(user, group, image_paths_json);
	}

	return false;
}

bool ImageGroupManager::ClearImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string user;
	std::string group;

	if (oJson.Get("user", user) && oJson.Get("group", group))
	{
		return ClearImages(user, group);
	}

	return false;
}

bool ImageGroupManager::AddImages(const std::string& user, const std::string& group, const std::set<std::string>& image_paths)
{
	std::string key = user + "/" + group;
	
	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		std::string img_paths;
		etcd_storage.GetValue(key, img_paths);

		//如果可以读取到etcd，则直接读etcd
		if (!img_paths.empty())
		{
			neb::CJsonObject oJson(img_paths);
			if (oJson.IsArray())
			{
				//转为 std::set 去重复数据
				std::set<std::string> images_set;
				int array_size = oJson.GetArraySize();

				for (int i = 0; i < array_size; i++)
				{
					std::string path;
					oJson.Get(i, path);
					images_set.emplace(path);
				}

				for (auto& image_path : image_paths)
				{
					images_set.emplace(image_path);
				}

				neb::CJsonObject oJson_array;
				for (auto& path : images_set)
				{
					oJson_array.Add(path);
				}
				
				return etcd_storage.SetValue(key, oJson_array.ToString(), false);
			}

			return false;
		}
		else
		{
			neb::CJsonObject oJson;
			for (auto& image_path : image_paths)
			{
				oJson.Add(image_path);
			}
			return etcd_storage.SetValue(key, oJson.ToString(), false);
		}
	}
	else
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		std::unordered_map<std::string, std::set<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr == s_user_group_image_map_.end())
		{
			s_user_group_image_map_.emplace(make_pair(key, image_paths));
		}
		else
		{
			for (auto& image_path : image_paths)
			{
				itr->second.insert(image_path);
			}
		}
	}

	return true;
}

bool ImageGroupManager::GetImages(const std::string& user, const std::string& group, std::string& images_json)
{
	std::string key = user + "/" + group;
	EtcdStorage etcd_storage;

	if (etcd_storage.IsUseEtcd())
	{
		if (etcd_storage.GetValue(key, images_json))
			return true;
	}
	else
	{
		std::shared_lock<std::shared_mutex> lock(s_mutex_);

		std::unordered_map<std::string, std::set<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr != s_user_group_image_map_.end())
		{
			neb::CJsonObject oJson;
			const std::set<std::string>& images = itr->second;
			for (auto& image : images)
			{
				oJson.Add(image);
			}

			images_json = oJson.ToString();
			return true;
		}
	}

	return false;
}

bool ImageGroupManager::ClearImages(const std::string& user, const std::string& group)
{
	std::string key = user + "/" + group;
	EtcdStorage etcd_storage;

	if (etcd_storage.IsUseEtcd())
	{
		return etcd_storage.Delete(key);
	}
	else
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		s_user_group_image_map_.erase(key);
	}

	return true;
}