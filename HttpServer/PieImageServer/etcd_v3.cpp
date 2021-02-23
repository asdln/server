#include "etcd_v3.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

EtcdV3::EtcdV3(const std::string address) : address_(address)
{

}

bool EtcdV3::SetValue(const std::string& key, const std::string& value, bool set_ttl)
{
	try
	{

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
// 		std::string value;
// 		HttpRequest(host_, port_, "/v2/keys/" + key, http::verb::delete_, value);
// 
// 		if (value.empty())
// 		{
// 			LOG(ERROR) << "etcd get value empty";
// 			return false;
// 		}
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