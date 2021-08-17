#include "style_map.h"
#include "etcd_storage.h"

std::shared_mutex StyleMap::s_mutex_;

std::unordered_map<std::string, std::string> StyleMap::style_map_;

std::string StyleMap::style_map_prefix_ = "style_map/";

#ifndef ETCD_V2

#include "etcd_v3.h"
#include "etcd/Client.hpp"
#include "etcd/Watcher.hpp"
#include "etcd/kv.pb.h"

std::unique_ptr<etcd::Watcher> style_map_watcher;

void wait_for_connection(etcd::Client& client);

void initialize_watcher(const std::string& endpoints,
	const std::string& prefix,
	std::function<void(etcd::Response)> callback,
	std::unique_ptr<etcd::Watcher>& watcher);

void PrintStyleMap(const std::unordered_map<std::string, std::string>& style_map)
{
	for (const auto& info : style_map)
	{
		std::cout << "key_value: " << info.first << "\t" << info.second << std::endl;
	}
}

void StyleMapCallback(etcd::Response response)
{
	//etcd::Response response = response_task.get(); // can throw
	if (!response.is_ok())
	{
		std::cout << "operation failed, details: " << response.error_message() << std::endl;
		return;
	}

	std::unique_lock<std::shared_mutex> lock(StyleMap::s_mutex_);

	//std::cout << "action: " << response.action() << std::endl;
	const std::string& key_full = response.value().key();
	const std::string& value = response.value().as_string();

	static int prefix_size = EtcdV3::GetPrefix().size();
	std::string key = key_full.substr(prefix_size, key_full.size() - prefix_size);

	if (response.action() == "create" || response.action() == "set")
	{
		StyleMap::style_map_[key] = value;
	}
	else if (response.action() == "delete")
	{
		std::unordered_map<std::string, std::string>::iterator itr = StyleMap::style_map_.find(key);
		if (itr != StyleMap::style_map_.end())
		{
			StyleMap::style_map_.erase(itr);
		}
	}

	//PrintStyleMap(StyleMap::style_map_);
}

#endif

void StyleMap::StartStyleWatch()
{
#ifndef ETCD_V2
	//设置etcd watcher
	if (EtcdStorage::IsUseEtcdV3())
	{
		//同步读取etcd里的style_map信息
		EtcdStorage etcd_storage;
		std::list<std::string> style_keys;
		etcd_storage.GetSubKeys(style_map_prefix_, style_keys);

		for (auto& style_key : style_keys)
		{
			std::string style_value;
			std::string key = style_map_prefix_ + style_key;
			etcd_storage.GetValue(key, style_value);

			style_map_[key] = style_value;
		}

		//PrintStyleMap(StyleMap::style_map_);

		//创建图层信息watcher
		const std::string& endpoints = EtcdStorage::GetV3Address();
		std::function<void(etcd::Response)> callback = StyleMapCallback;
		std::string prefix = EtcdV3::GetPrefix() + style_map_prefix_;
		prefix = prefix.substr(0, prefix.size() - 1);

		// the watcher initialized in this way will auto re-connect to etcd
		initialize_watcher(endpoints, prefix, callback, style_map_watcher);
	}
#endif
}

void StyleMap::Update(const std::string& key, const std::string& value)
{
	std::unique_lock<std::shared_mutex> lock(s_mutex_);

	EtcdStorage etcd_storage;
	if(etcd_storage.IsUseEtcd())
	{
		etcd_storage.SetValue(style_map_prefix_ + key, value);
	}
	else
	{
		std::unordered_map<std::string, std::string>::iterator itr = style_map_.find(style_map_prefix_ + key);
		if (itr != style_map_.end())
		{
			style_map_.erase(itr);
		}

		style_map_.emplace(style_map_prefix_ + key, value);
	}
}

bool StyleMap::Find(const std::string& key, std::string& value)
{
	std::shared_lock<std::shared_mutex> lock(s_mutex_);

	std::unordered_map<std::string, std::string>::iterator itr = style_map_.find(style_map_prefix_ + key);
	if (itr != style_map_.end())
	{
		value = itr->second;
		return true;
	}
	
	return false;
}