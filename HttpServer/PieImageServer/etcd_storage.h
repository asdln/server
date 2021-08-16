#pragma once

#include <string>
#include <list>

class EtcdStorage
{
	friend class Application;

public:

	EtcdStorage();

	bool SetValue(const std::string& key, const std::string& value, bool set_ttl = true);

	bool GetValue(const std::string& key, std::string& value);

	bool Delete(const std::string& key);

    bool GetSubKeys(const std::string& key, std::list<std::string>& sub_keys);

    bool IsUseEtcd() { return use_etcd_v2_ || use_etcd_v3_; }

    static bool IsUseEtcdV3() { return use_etcd_v3_; }

    static const std::string& GetV3Address() { return address_v3_; }

protected:

    // use for v2
    static std::string host_v2_;
    static std::string port_v2_;

    // use for v3
    static std::string address_v3_;

    static bool use_etcd_v2_;

    static bool use_etcd_v3_;
};
