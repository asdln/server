#pragma once

#ifndef ETCD_V2

#include <string>

class EtcdV3
{
public:

	EtcdV3(const std::string address);

	bool SetValue(const std::string& key, const std::string& value, bool set_ttl = true);

	bool GetValue(const std::string& key, std::string& value);

	bool Delete(const std::string& key);

	const std::string GetPrefix() { return prefix_; }

protected:

	std::string address_;  //http://127.0.0.1:4001

    std::string prefix_ = "/pie_image_server/";
};

#endif
