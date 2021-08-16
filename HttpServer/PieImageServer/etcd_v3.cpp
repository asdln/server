#include "etcd_v3.h"

#ifndef ETCD_V2

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#include "etcd/Client.hpp"
#include "etcd/kv.pb.h"

std::mutex mutex_client;
etcd::Client* g_client = nullptr;

etcd::Client& GetEtcdClient(const std::string& address)
{
	if (g_client != nullptr)
		return *g_client;

	{
		std::lock_guard<std::mutex> guard(mutex_client);
		if (g_client != nullptr)
		{
			return *g_client;
		}

		g_client = new etcd::Client(address);
		return *g_client;
	}
}

std::string EtcdV3::prefix_ = "/pie_image_server/";

EtcdV3::EtcdV3(const std::string& address) : address_(address)
{

}

bool EtcdV3::SetValue(const std::string& key, const std::string& value, bool set_ttl)
{
	try
	{
		//etcd::Client etcd(address_);
		etcd::Client& etcd = GetEtcdClient(address_);
		pplx::task<etcd::Response> response_task;

		if (set_ttl)
		{
			etcd::Response resp = etcd.leasegrant(18000).get();
            response_task = etcd.set(prefix_ + key, value, resp.value().lease());
		}
		else
		{
            response_task = etcd.set(prefix_ + key, value);
		}

		etcd::Response response = response_task.get();
		if (!response.is_ok())
		{
			LOG(ERROR) << "operation failed, details: " << response.error_message();
            return false;
		}
	}
	catch (std::exception e)
	{
		LOG(ERROR) << "EtcdV3 SetValue failed" << e.what();
		return false;
	}
	catch (...)
	{
		LOG(ERROR) << "EtcdV3 SetValue failed";
		return false;
	}

	return true;
}

bool EtcdV3::GetValue(const std::string& key, std::string& value)
{
	try
	{
        //etcd::Client etcd(address_);
		etcd::Client& etcd = GetEtcdClient(address_);
        pplx::task<etcd::Response> response_task = etcd.get(prefix_ + key);

		etcd::Response response = response_task.get(); // can throw
		if (!response.is_ok())
		{
			LOG(ERROR) << "operation failed, details: " << response.error_message();
			LOG(ERROR) << "key:  " << prefix_ + key;
			return false;
		}

		value = response.value().as_string();
	}
	catch (std::exception e)
	{
		LOG(ERROR) << "EtcdV3 GetValue failed" << e.what();
		return false;
	}
	catch (...)
	{
		LOG(ERROR) << "EtcdV3 GetValue failed";
		return false;
	}

	return true;
}

bool EtcdV3::Delete(const std::string& key)
{
	try
	{
        //etcd::Client etcd(address_);
		etcd::Client& etcd = GetEtcdClient(address_);
        pplx::task<etcd::Response> response_task = etcd.rm(prefix_ + key);

        etcd::Response response = response_task.get(); // can throw
        if (!response.is_ok())
        {
            LOG(ERROR) << "operation failed, details: " << response.error_message();
            return false;
        }
	}
	catch (std::exception e)
	{
		LOG(ERROR) << "Etcd SetValue failed" << e.what();
		return false;
	}
	catch (...)
	{
		LOG(ERROR) << "Etcd GetValue failed";
		return false;
	}

	return true;
}

bool EtcdV3::GetSubKeys(const std::string& key, std::list<std::string>& sub_keys)
{
	try
	{
		etcd::Client& etcd = GetEtcdClient(address_);
		std::string prefix = /*"/test/key"*/prefix_ + key;

		int prefix_size = prefix.size();
		std::unordered_map<std::string, std::list<std::string>> group_image_map;
		etcd::Response resp = etcd.ls(prefix).get();

		for (int i = 0; i < resp.keys().size(); ++i)
		{
			std::string key_raw = resp.key(i);
			std::string group = key_raw.substr(prefix_size, key_raw.size() - prefix_size);
			sub_keys.emplace_back(group);
		}
	}
	catch (...)
	{
		LOG(ERROR) << "Etcd GetSubKeys failed";
		return false;
	}

	return true;
}

#endif
