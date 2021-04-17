#pragma once

#include <string>
#include <boost/beast/http.hpp>

class EtcdV2
{
public:
    EtcdV2(std::string host, std::string port);

    bool SetValue(const std::string& key, const std::string& value, bool set_ttl = true);

    bool GetValue(const std::string& key, std::string& value);

    bool Delete(const std::string& key);

protected:

    void HttpRequest(const std::string& host, const std::string& port, const std::string& target, boost::beast::http::verb op, std::string& value);

protected:

    std::string host_;

    std::string port_;
};
