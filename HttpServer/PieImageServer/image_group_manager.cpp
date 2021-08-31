#include "image_group_manager.h"
#include "etcd_storage.h"
#include "CJsonObject.hpp"
#include "etcd_storage.h"
#include <filesystem>
#include <iostream>

#include "file_cache.h"
#include "amazon_s3.h"

std::shared_mutex ImageGroupManager::s_mutex_;

std::shared_mutex ImageGroupManager::s_group_cache_mutex_;

std::string ImageGroupManager::image_group_prefix_ = "image_groups/";

std::string ImageGroupManager::group_cache_prefix_ = "image_groups_state/";

std::unordered_map<std::string, std::list<std::string>> ImageGroupManager::s_user_group_image_map_;

std::unordered_map<std::string, bool> ImageGroupManager::s_group_cache_state_;

#ifndef ETCD_V2

#include "etcd_v3.h"
#include "etcd/Client.hpp"
#include "etcd/Watcher.hpp"
#include "etcd/kv.pb.h"

std::unique_ptr<etcd::Watcher> image_group_watcher;

std::unique_ptr<etcd::Watcher> group_cache_watcher;

void PrintMap(const std::unordered_map<std::string, std::list<std::string>>& unordered_map)
{
	for (const auto& info : unordered_map)
	{
		std::cout << info.first << std::endl;
		const std::list<std::string>& paths = info.second;

		for (const auto& path : paths)
		{
			std::cout << path << "\t";
		}
		std::cout << std::endl << std::endl;
	}
}

void imagesResponse(etcd::Response response)
{
	//etcd::Response response = response_task.get(); // can throw
	if (!response.is_ok())
	{
		std::cout << "operation failed, details: " << response.error_message() << std::endl;
		return;
	}

	std::unique_lock<std::shared_mutex> lock(ImageGroupManager::s_mutex_);

	//std::cout << "action: " << response.action() << std::endl;
	const std::string& key_full = response.value().key();
	const std::string& value = response.value().as_string();

	static int prefix_size = EtcdV3::GetPrefix().size();
	std::string key = key_full.substr(prefix_size, key_full.size() - prefix_size);

	if (response.action() == "create" || response.action() == "set")
	{
		neb::CJsonObject oJson(value);
		//std::cout << "value: " << value << std::endl;
		if (oJson.IsArray())
		{
			std::string image;
			std::list<std::string> images;

			int arrar_size = oJson.GetArraySize();
			for (int i = 0; i < arrar_size; i++)
			{
				oJson.Get(i, image);
				images.emplace_back(image);
			}

			ImageGroupManager::s_user_group_image_map_[key] = images;
			//PrintMap(ImageGroupManager::s_user_group_image_map_);
		}
	}
	else if (response.action() == "delete")
	{
		std::unordered_map<std::string, std::list<std::string>>::iterator itr = ImageGroupManager::s_user_group_image_map_.find(key);
		if (itr != ImageGroupManager::s_user_group_image_map_.end())
		{
			ImageGroupManager::s_user_group_image_map_.erase(itr);
		}
	}
}

void cacheStateResponse(etcd::Response response)
{
	//etcd::Response response = response_task.get(); // can throw
	if (!response.is_ok())
	{
		std::cout << "operation failed, details: " << response.error_message() << std::endl;
		return;
	}

	std::unique_lock<std::shared_mutex> lock(ImageGroupManager::s_group_cache_mutex_);

	//std::cout << "action: " << response.action() << std::endl;
	const std::string& key_full = response.value().key();
	const std::string& value = response.value().as_string();

	static int prefix_size = EtcdV3::GetPrefix().size();
	std::string key = key_full.substr(prefix_size, key_full.size() - prefix_size);

	if (response.action() == "create" || response.action() == "set")
	{
		bool state = true;
		if (value.compare("0") == 0)
		{
			state = false;
		}

		ImageGroupManager::s_group_cache_state_[key] = state;
	}
	else if (response.action() == "delete")
	{
		std::unordered_map<std::string, bool>::iterator itr = ImageGroupManager::s_group_cache_state_.find(key);
		if (itr != ImageGroupManager::s_group_cache_state_.end())
		{
			ImageGroupManager::s_group_cache_state_.erase(itr);
		}
	}
}

void wait_for_connection(etcd::Client& client) {
	// wait until the client connects to etcd server 
	// `head` API is only available in version later than 0.2.1
	while (!client.head().get().is_ok()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::cout << "reconnect to etcd after 1 second" << std::endl;
	}

	std::cout << "etcd watcher connected" << std::endl;
}

void initialize_watcher(const std::string& endpoints,
	const std::string& prefix,
	std::function<void(etcd::Response)> callback,
	std::unique_ptr<etcd::Watcher>& watcher) {
	etcd::Client client(endpoints);
	wait_for_connection(client);
	watcher.reset(new etcd::Watcher(client, prefix, callback, true));
	watcher->Wait([endpoints, prefix, callback,
		/*watcher_ref*/ /* keep the shared_ptr alive */ &watcher](bool cancelled) {
			if (cancelled) {
				return;
			}
			initialize_watcher(endpoints, prefix, callback, watcher);
		});
}

#endif

void ImageGroupManager::StartImageGroupWatch()
{
	StartImagesWatch();
	StartCacheStateWatch();
}

void ImageGroupManager::StartImagesWatch()
{
#ifndef ETCD_V2
	//设置etcd watcher
	if (EtcdStorage::IsUseEtcdV3())
	{
		//同步读取etcd里的图层信息
		EtcdStorage etcd_storage;
		std::list<std::string> groups;
		etcd_storage.GetSubKeys(image_group_prefix_, groups);

		for (auto& group : groups)
		{
			std::string images;
			std::string key = image_group_prefix_ + group;
			etcd_storage.GetValue(key, images);

			std::list<std::string> list_image;
			neb::CJsonObject oJson(images);
			if (oJson.IsArray())
			{
				int arrar_size = oJson.GetArraySize();
				for (int i = 0; i < arrar_size; i++)
				{
					std::string temp;
					oJson.Get(i, temp);
					list_image.emplace_back(temp);
				}
			}

			s_user_group_image_map_[key] = list_image;
		}

		//创建图层信息watcher
		const std::string& endpoints = EtcdStorage::GetV3Address();
		std::function<void(etcd::Response)> callback = imagesResponse;
		std::string prefix = EtcdV3::GetPrefix() + image_group_prefix_;
		prefix = prefix.substr(0, prefix.size() - 1);

		// the watcher initialized in this way will auto re-connect to etcd
		initialize_watcher(endpoints, prefix, callback, image_group_watcher);
	}
#endif
}

void ImageGroupManager::StartCacheStateWatch()
{
#ifndef ETCD_V2
	//设置etcd watcher
	if (EtcdStorage::IsUseEtcdV3())
	{
		//同步读取etcd里的图层状态信息
		EtcdStorage etcd_storage;
		std::list<std::string> groups;
		etcd_storage.GetSubKeys(group_cache_prefix_, groups);

		for (auto& group : groups)
		{
			std::string state_str;
			std::string key = group_cache_prefix_ + group;
			etcd_storage.GetValue(key, state_str);

			bool state = true;
			if (state_str.compare("0") == 0)
			{
				state = false;
			}

			s_group_cache_state_[key] = state;
		}

		//创建图层信息watcher
		const std::string& endpoints = EtcdStorage::GetV3Address();
		std::function<void(etcd::Response)> callback = cacheStateResponse;
		std::string prefix = EtcdV3::GetPrefix() + group_cache_prefix_;
		prefix = prefix.substr(0, prefix.size() - 1);

		// the watcher initialized in this way will auto re-connect to etcd
		initialize_watcher(endpoints, prefix, callback, group_cache_watcher);
	}
#endif
}

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

bool ImageGroupManager::GetImagesFromLocal(const std::string& group, std::list<std::string>& image_paths)
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

bool ImageGroupManager::AddGroups(const std::string& request_body)
{
	EtcdStorage etcd_storage;
	neb::CJsonObject oJson(request_body);
	if (oJson.IsArray())
	{
		int arrar_size = oJson.GetArraySize();
		for (int i = 0; i < arrar_size; i++)
		{
			std::string group;
			oJson.Get(i, group);

			std::string key = image_group_prefix_ + group;
			if (etcd_storage.IsUseEtcd())
			{
				etcd_storage.SetValue(key, "", false);
			}
			else
			{
				std::unique_lock<std::shared_mutex> lock(s_mutex_);
				std::list<std::string> temp;
				s_user_group_image_map_[key] = temp;
			}
		}
		//images_obj.GetArraySize()
	}

	return true;
}

bool ImageGroupManager::RemoveGroups(const std::string& request_body)
{
	EtcdStorage etcd_storage;
	neb::CJsonObject oJson(request_body);
	if (oJson.IsArray())
	{
		int arrar_size = oJson.GetArraySize();
		for (int i = 0; i < arrar_size; i++)
		{
			std::string group;
			oJson.Get(i, group);

			std::string key = image_group_prefix_ + group;

			if (etcd_storage.IsUseEtcd())
			{
				etcd_storage.Delete(key);
			}
			else
			{
				std::unique_lock<std::shared_mutex> lock(s_mutex_);

				std::unordered_map<std::string, std::list<std::string>>::iterator itr
					= s_user_group_image_map_.find(key);

				if (itr != s_user_group_image_map_.end())
					s_user_group_image_map_.erase(itr);
			}
		}
	}

	return true;
}

void ImageGroupManager::GetAllGroupsInternal(std::list<std::string>& groups)
{
	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
	{
		std::string key = image_group_prefix_;
		etcd_storage.GetSubKeys(key, groups);
	}
	else
	{
		int size = image_group_prefix_.size();
		std::shared_lock<std::shared_mutex> lock(s_mutex_);
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
		return etcd_storage.SetValue(key, "", false);
	}
	else
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);
		std::unordered_map<std::string, std::list<std::string>>::iterator itr = s_user_group_image_map_.find(key);
		if (itr != s_user_group_image_map_.end())
		{
			itr->second.clear();
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool ImageGroupManager::SetGroupCacheState(const std::string& request_body)
{
	bool total_state = S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache();
	if (total_state == false)
		return false;

	neb::CJsonObject oJson(request_body);
	if (oJson.IsArray())
	{
		EtcdStorage etcd_storage;
		int array_size = oJson.GetArraySize();
		for (int i = 0; i < array_size; i ++)
		{
			neb::CJsonObject group_state;
			oJson.Get(i, group_state);

			if(group_state.IsEmpty())
				continue;

			bool state;
			std::string group;
			
			group_state.Get("group", group);
			group_state.Get("cache", state);

			std::string key = group_cache_prefix_ + group;
			if (etcd_storage.IsUseEtcd())
			{
				std::string state_str = "1";
				if (state == false)
					state_str = "0";

				etcd_storage.SetValue(key, state_str, false);
			}
			else
			{
				std::unique_lock<std::shared_mutex> lock(s_group_cache_mutex_);
				s_group_cache_state_[key] = state;
			}
		}

		return true;
	}

	return false;
}

bool ImageGroupManager::GetGroupCacheState(const std::string& group)
{
	//因为已经使用了etcd watcher进行了同步，所以只需要从map中读取。
	std::string key = group_cache_prefix_ + group;
	std::shared_lock<std::shared_mutex> lock(s_group_cache_mutex_);
	std::unordered_map<std::string, bool>::iterator itr = s_group_cache_state_.find(key);

	if (itr != s_group_cache_state_.end())
	{
		return itr->second;
	}

	return true;
}

bool ImageGroupManager::GetGroupCacheState(const std::string& request_body, std::string& state_json)
{
	neb::CJsonObject oJson(request_body);
	std::list<bool> states;

	bool total_state = S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache();

	if (oJson.IsArray())
	{
		int array_size = oJson.GetArraySize();

		for (int i = 0; i < array_size; i++)
		{
			std::string group;
			oJson.Get(i, group);

			if (total_state == false)
			{
				states.emplace_back(false);
			}
			else
			{
				bool state = GetGroupCacheState(group);
				states.emplace_back(state);
			}
		}
	}

	neb::CJsonObject oJson_states;
	for (const auto& state : states)
	{
		oJson_states.Add(999, state);
	}

	state_json = oJson_states.ToString();

	return true;
}

bool ImageGroupManager::ClearGroupCache(const std::string& request_body)
{
	neb::CJsonObject oJson(request_body);
	if (oJson.IsArray())
	{
		EtcdStorage etcd_storage;
		int array_size = oJson.GetArraySize();
		for (int i = 0; i < array_size; i++)
		{
			std::string group;
			oJson.Get(i, group);

			if (S3Cache::GetUseS3Cache())
			{
				//test code
				std::cout << "s3 delete object" << std::endl;
				S3Cache::DeleteObject(group);
			}
			else if (FileCache::GetUseFileCache())
			{
				try
				{
					const std::string& file_cache_dir = FileCache::GetSavePath();
					std::filesystem::path b_dir_path(file_cache_dir + '/' + group);
					if (std::filesystem::exists(b_dir_path))
					{
						std::filesystem::remove_all(b_dir_path);
					}
				}
				catch (std::filesystem::filesystem_error& e)
				{
					std::cout << "what=" << e.what() << "code=" << e.code().value() << e.code().message() << std::endl;
					return false;
				}
			}
		}

		return true;
	}

	return false;
}