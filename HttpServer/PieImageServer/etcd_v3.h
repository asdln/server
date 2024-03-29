#pragma once

#ifndef ETCD_V2

#include <string>
#include <list>

class EtcdV3
{
public:

	EtcdV3(const std::string& address);

	//ttl ��λ Ϊ ��
	bool SetValue(const std::string& key, const std::string& value, bool set_ttl, int ttl);

	bool GetValue(const std::string& key, std::string& value);

	bool Delete(const std::string& key);

	bool GetSubKeys(const std::string& key, std::list<std::string>& sub_keys);

	bool Lock(const std::string& key);

	bool Unlock(const std::string& key);

	static const std::string GetPrefix() { return prefix_; }

protected:

	const std::string& address_;  //http://127.0.0.1:4001

    static std::string prefix_;
};

#endif
