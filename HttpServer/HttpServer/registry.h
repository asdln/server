#ifndef HTTPSERVER_REGISTRY_H_
#define HTTPSERVER_REGISTRY_H_

#include <string>

class Registry
{
public:

	static std::string etcd_host_;

	static std::string etcd_port_;
};

#endif //HTTPSERVER_REGISTRY_H_