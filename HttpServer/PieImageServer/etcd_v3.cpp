#include "etcd_v3.h"

#ifndef ETCD_V2

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#include "etcd/Client.hpp"
#include "etcd/kv.pb.h"

EtcdV3::EtcdV3(const std::string address) : address_(address)
{

}

bool EtcdV3::SetValue(const std::string& key, const std::string& value, bool set_ttl)
{
	try
	{
		etcd::Client etcd(address_);
		pplx::task<etcd::Response> response_task;

		if (set_ttl)
		{
			etcd::Response resp = etcd.leasegrant(18000).get();
            response_task = etcd.set(prefix_ + "/" + key, value, resp.value().lease());
		}
		else
		{
            response_task = etcd.set(prefix_ + "/" + key, value);
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
        etcd::Client etcd(address_);
        pplx::task<etcd::Response> response_task = etcd.get(prefix_ + "/" + key);

		etcd::Response response = response_task.get(); // can throw
		if (!response.is_ok())
		{
			LOG(ERROR) << "operation failed, details: " << response.error_message();
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
        etcd::Client etcd(address_);
        pplx::task<etcd::Response> response_task = etcd.rm(prefix_ + "/" + key);

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

#endif
