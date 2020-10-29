#ifndef HTTPSERVER_ETCD_STORAGE_H_
#define HTTPSERVER_ETCD_STORAGE_H_

#include <string>
#include <boost/beast/http.hpp>

class EtcdStorage
{
	friend class Application;

public:

	EtcdStorage();

	EtcdStorage(const std::string& host, const std::string& port = "2379");

	
	void SetValue(const std::string& key, const std::string& value);

	std::string GetValue(const std::string& key);

protected:

	void HttpRequest(const std::string& host, const std::string& port, const std::string& target, boost::beast::http::verb op, std::string& value);


protected:

	static std::string host_;

	static std::string port_;
};

#endif  //HTTPSERVER_ETCD_STORAGE_H_