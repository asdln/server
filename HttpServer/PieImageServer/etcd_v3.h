#ifndef HTTPSERVER_ETCD_V3_H_
#define HTTPSERVER_ETCD_V3_H_

#include <string>

class EtcdV3
{
public:

	EtcdV3(const std::string address);

	bool SetValue(const std::string& key, const std::string& value, bool set_ttl = true);

	bool GetValue(const std::string& key, std::string& value);

	bool Delete(const std::string& key);

protected:

	std::string address_;  //http://127.0.0.1:4001
};

#endif