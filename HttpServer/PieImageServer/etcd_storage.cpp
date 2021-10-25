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

bool EtcdStorage::GetSubKeys(const std::string& key, std::list<std::string>& sub_keys)
{
#ifdef ETCD_V2
    return false;
#else
	EtcdV3 etcdv3(address_v3_);
	return etcdv3.GetSubKeys(key, sub_keys);
#endif
}

bool EtcdStorage::Lock(const std::string& key)
{
#ifdef ETCD_V2
    return false;
#else
	EtcdV3 etcdv3(address_v3_);
	return etcdv3.Lock(key);
#endif
}

bool EtcdStorage::Unlock(const std::string& key)
{
#ifdef ETCD_V2
	return false;
#else
	EtcdV3 etcdv3(address_v3_);
	return etcdv3.Unlock(key);
#endif
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
