#include "etcd_storage.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <sstream>
#include "CJsonObject.hpp"
#include "registry.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

EtcdStorage::EtcdStorage()
{
	host_ = Registry::etcd_host_;
	port_ = Registry::etcd_port_;
}

EtcdStorage::EtcdStorage(const std::string& host, const std::string& port)
{
	host_ = host;
	port_ = port;
}

void EtcdStorage::SetValue(const std::string& key, const std::string& value)
{
	try
	{
		std::string value1;
		HttpRequest(host_, port_, "/v2/keys" + key + "?value=" + value, http::verb::put, value1);
	}
	catch (std::exception e)
	{
		LOG(ERROR) << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "Etcd SetValue failed";
	}
}

std::string EtcdStorage::GetValue(const std::string& key)
{
	std::string res;

	try
	{
		std::string value;
		HttpRequest(host_, port_, "/v2/keys" + key, http::verb::get, value);

		if (value.empty())
			return "";

		neb::CJsonObject oJson(value);
		res = oJson["node"]("value");
	}
	catch (std::exception e)
	{
		LOG(ERROR) << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "Etcd GetValue failed";
	}

	return res;
}

void EtcdStorage::HttpRequest(const std::string& host, const std::string& port, const std::string& target, http::verb op, std::string& value)
{
	int version = 11;

	// The io_context is required for all I/O
	net::io_context ioc;

	// These objects perform our I/O
	tcp::resolver resolver(ioc);
	beast::tcp_stream stream(ioc);

	// Look up the domain name
	auto const results = resolver.resolve(host, port);

	// Make the connection on the IP address we get from a lookup
	stream.connect(results);

	// Set up an HTTP GET request message
	http::request<http::string_body> req{ op, target, version };
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// Send the HTTP request to the remote host
	http::write(stream, req);

	// This buffer is used for reading and must be persisted
	beast::flat_buffer buffer;

	// Declare a container to hold the response
	//http::response<http::dynamic_body> res;
	http::response<http::string_body> res;

	// Receive the HTTP response
	http::read(stream, buffer, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	std::stringstream osstream;
	osstream << res.body();
	value = osstream.str();

	// Gracefully close the socket
	beast::error_code ec;
	stream.socket().shutdown(tcp::socket::shutdown_both, ec);

	// not_connected happens sometimes
	// so don't bother reporting it.
	//
	if (ec && ec != beast::errc::not_connected)
		throw beast::system_error{ ec };

	// If we get here then the connection is closed gracefully
}