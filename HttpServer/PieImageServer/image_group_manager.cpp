#include "image_group_manager.h"
#include "etcd_storage.h"
#include "CJsonObject.hpp"

std::shared_mutex ImageGroupManager::s_mutex_;

std::string ImageGroupManager::image_group_prefix_ = "image_groups/";

std::unordered_map<std::string, std::list<std::string>> ImageGroupManager::s_user_group_image_map_;

std::unordered_map<std::string, std::list<std::string>> ImageGroupManager::s_etcd_cache_group_image_map_;

std::shared_mutex ImageGroupManager::s_etcd_cache_mutex_;

bool ImageGroupManager::AddImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string group;
	std::list<std::string> image_paths;
	if (oJson.Get("group", group))
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
					image_paths.push_back(path);
				}

				return AddImagesInternal(group, image_paths);
			}
		}
	}

	return false;
}

bool ImageGroupManager::RemoveImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string group;
	std::list<std::string> image_paths;
	if (oJson.Get("group", group))
	{
		neb::CJsonObject images_obj;
		if (oJson.Get("images", images_obj))
		{
			if (images_obj.IsArray())
			{
				int array_size = images_obj.GetArraySize();
				for (int i = 0; i < array_size; i++)
				{
					std::string path;
					images_obj.Get(i, path);
					image_paths.push_back(path);
				}

				return RemoveImagesInternal(group, image_paths);
			}
		}
	}

	return false;
}

bool ImageGroupManager::SetImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string group;
	std::list<std::string> image_paths;
	if (oJson.Get("group", group))
	{
		neb::CJsonObject images_obj;
		if (oJson.Get("images", images_obj))
		{
			if (images_obj.IsArray())
			{
				int array_size = images_obj.GetArraySize();
				for (int i = 0; i < array_size; i++)
				{
					std::string path;
					images_obj.Get(i, path);
					image_paths.push_back(path);
				}

				return SetImagesInternal(group, image_paths);
			}
		}
	}

	return false;
}

bool ImageGroupManager::GetImages(const std::string& request_body, std::string& image_paths_json)
{
	neb::CJsonObject oJson(request_body);

	std::string group;
	
	if (oJson.Get("group", group))
	{
		return GetImagesInternal(group, image_paths_json);
	}

	return false;
}

bool ImageGroupManager::GetImages(const std::string& group, std::list<std::string>& image_paths)
{
	std::string key = image_group_prefix_ + group;
	std::shared_lock<std::shared_mutex> lock(s_mutex_);

	std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
	if (itr != s_user_group_image_map_.end())
	{
		image_paths = itr->second;
	}

	return true;
}

bool ImageGroupManager::GetAllGroup(std::string& groups_json)
{
	std::list<std::string> groups;
	GetAllGroupsInternal(groups);

	neb::CJsonObject oJson_array;
	for (auto& path : groups)
	{
		oJson_array.Add(path);
	}

	groups_json = oJson_array.ToString();

	return true;
}

void ImageGroupManager::GetAllGroupsInternal(std::list<std::string>& groups)
{
	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		etcd_storage.GetGroups(groups);
	}
	else
	{
		int size = image_group_prefix_.size();
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		for (const auto& group : s_user_group_image_map_)
		{
			std::string raw_group = group.first;
			raw_group = raw_group.substr(size, raw_group.size() - size);
			groups.emplace_back(raw_group);
		}
	}
}

bool ImageGroupManager::ClearImages(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);

	std::string group;

	if (oJson.Get("group", group))
	{
		return ClearImagesInternal(group);
	}

	return false;
}

bool ImageGroupManager::AddImagesInternal(const std::string& group, const std::list<std::string>& image_paths)
{
	std::string key = image_group_prefix_ + group;
	
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
				//直接添加，不去掉重复
				std::list<std::string> images_list;
				int array_size = oJson.GetArraySize();

				for (int i = 0; i < array_size; i++)
				{
					std::string path;
					oJson.Get(i, path);
					images_list.emplace_back(path);
				}

				for (auto& image_path : image_paths)
				{
					images_list.emplace_back(image_path);
				}

				neb::CJsonObject oJson_array;
				for (auto& path : images_list)
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
		std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr == s_user_group_image_map_.end())
		{
			s_user_group_image_map_.emplace(make_pair(key, image_paths));
		}
		else
		{
			for (auto& image_path : image_paths)
			{
				itr->second.push_back(image_path);
			}
		}
	}

	return true;
}

bool ImageGroupManager::RemoveImagesInternal(const std::string& group, const std::list<std::string>& image_paths)
{
	std::string key = image_group_prefix_ + group;

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
				//直接添加，不去掉重复
				std::list<std::string> images_list;
				int array_size = oJson.GetArraySize();

				for (int i = 0; i < array_size; i++)
				{
					std::string path;
					oJson.Get(i, path);
					images_list.emplace_back(path);
				}

				for (auto& image_path : image_paths)
				{
					images_list.remove(image_path);
				}

				neb::CJsonObject oJson_array;
				for (auto& path : images_list)
				{
					oJson_array.Add(path);
				}

				return etcd_storage.SetValue(key, oJson_array.ToString(), false);
			}

			return false;
		}
	}
	else
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr != s_user_group_image_map_.end())
		{
			for (auto& image_path : image_paths)
			{
				itr->second.remove(image_path);
			}
		}
	}

	return true;
}

bool ImageGroupManager::SetImagesInternal(const std::string& group, const std::list<std::string>& image_paths)
{
	std::string key = image_group_prefix_ + group;

	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		neb::CJsonObject oJson;
		for (auto& image_path : image_paths)
		{
			oJson.Add(image_path);
		}
		return etcd_storage.SetValue(key, oJson.ToString(), false);
		
	}
	else
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr == s_user_group_image_map_.end())
		{
			s_user_group_image_map_.emplace(make_pair(key, image_paths));
		}
		else
		{
			itr->second.clear();
			for (auto& image_path : image_paths)
			{
				itr->second.push_back(image_path);
			}
		}
	}

	return true;
}

bool ImageGroupManager::GetImagesInternal(const std::string& group, std::string& images_json)
{
	std::string key = image_group_prefix_ + group;
	EtcdStorage etcd_storage;

	if (etcd_storage.IsUseEtcd())
	{
		if (etcd_storage.GetValue(key, images_json))
			return true;
	}
	else
	{
		std::shared_lock<std::shared_mutex> lock(s_mutex_);

		std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr != s_user_group_image_map_.end())
		{
			neb::CJsonObject oJson;
			const std::list<std::string>& images = itr->second;
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

bool ImageGroupManager::ClearImagesInternal(const std::string& group)
{
	std::string key = image_group_prefix_ + group;
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