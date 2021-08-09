#include "etcd_storage.h"

//#define ETCD_V2

#ifdef ETCD_V2
#include "etcd_v2.h"
#else
#include "etcd_v3.h"
#include "etcd/Client.hpp"
#include "etcd/kv.pb.h"
#endif

#include "image_group_manager.h"

std::string EtcdStorage::host_v2_ = "0.0.0.0";

std::string EtcdStorage::port_v2_ = "2379";

std::string EtcdStorage::address_v3_ = "http://127.0.0.1:2379";

bool EtcdStorage::use_etcd_v2_ = false;
bool EtcdStorage::use_etcd_v3_ = false;

EtcdStorage::EtcdStorage()
{
}

bool EtcdStorage::SetValue(const std::string& key, const std::string& value, bool set_ttl)
{
#ifdef ETCD_V2
    EtcdV2 etcdv2(host_v2_, port_v2_);
    return etcdv2.SetValue(key, value, set_ttl);
#else
    EtcdV3 etcdv3(address_v3_);
    return etcdv3.SetValue(key, value, set_ttl);
#endif
}

void EtcdStorage::GetGroups(std::list<std::string>& groups)
{
#ifndef ETCD_V2

	EtcdV3 etcdv3(address_v3_);
	etcd::Client etcd(address_v3_);

	std::string prefix = /*"/test/key"*/etcdv3.GetPrefix() + ImageGroupManager::GetPrefix();
	prefix = prefix.substr(0, prefix.size() - 1);
	int prefix_size = prefix.size() + 1; // + 1 “™À„…œ °∞/°±

	std::unordered_map<std::string, std::list<std::string>> group_image_map;

	etcd::Response resp = etcd.ls(prefix).get();

	for (int i = 0; i < resp.keys().size(); ++i)
	{
		std::string key_raw = resp.key(i);
		std::string group = key_raw.substr(prefix_size, key_raw.size() - prefix_size);
		groups.emplace_back(group);
	}

#endif // ETCD_V2
}

bool EtcdStorage::GetValue(const std::string& key, std::string& value)
{
#ifdef ETCD_V2
    EtcdV2 etcdv2(host_v2_, port_v2_);
    return etcdv2.GetValue(key, value);
#else
    EtcdV3 etcdv3(address_v3_);
    return etcdv3.GetValue(key, value);
#endif

}

bool EtcdStorage::Delete(const std::string& key)
{
#ifdef ETCD_V2
    EtcdV2 etcdv2(host_v2_, port_v2_);
    return etcdv2.Delete(key);
#else
    EtcdV3 etcdv3(address_v3_);
    return etcdv3.Delete(key);
#endif
}
