﻿#include "application.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gdal_priv.h"
#include "listener.h"

#include <io.h>
#include <fstream>

namespace fs = boost::filesystem;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

ApplicationPtr global_app = nullptr;

// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
	using beast::iequals;
	auto const ext = [&path]
	{
		auto const pos = path.rfind(".");
		if (pos == beast::string_view::npos)
			return beast::string_view{};
		return path.substr(pos);
	}();
	if (iequals(ext, ".htm"))  return "text/html";
	if (iequals(ext, ".html")) return "text/html";
	if (iequals(ext, ".php"))  return "text/html";
	if (iequals(ext, ".css"))  return "text/css";
	if (iequals(ext, ".txt"))  return "text/plain";
	if (iequals(ext, ".js"))   return "application/javascript";
	if (iequals(ext, ".json")) return "application/json";
	if (iequals(ext, ".xml"))  return "application/xml";
	if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
	if (iequals(ext, ".flv"))  return "video/x-flv";
	if (iequals(ext, ".png"))  return "image/png";
	if (iequals(ext, ".jpe"))  return "image/jpeg";
	if (iequals(ext, ".jpeg")) return "image/jpeg";
	if (iequals(ext, ".jpg"))  return "image/jpeg";
	if (iequals(ext, ".gif"))  return "image/gif";
	if (iequals(ext, ".bmp"))  return "image/bmp";
	if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
	if (iequals(ext, ".tiff")) return "image/tiff";
	if (iequals(ext, ".tif"))  return "image/tiff";
	if (iequals(ext, ".svg"))  return "image/svg+xml";
	if (iequals(ext, ".svgz")) return "image/svg+xml";
	return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
	beast::string_view base,
	beast::string_view path)
{
	if (base.empty())
		return std::string(path);
	std::string result(base);
#ifdef BOOST_MSVC
	char constexpr path_separator = '\\';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
	for (auto& c : result)
		if (c == '/')
			c = path_separator;
#else
	char constexpr path_separator = '/';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
#endif
	return result;
}

int get_filenames(const std::string& dir, std::list<std::string>& filenames)
{
	fs::path path(dir);
	if (!fs::exists(path))
	{
		return -1;
	}

	fs::directory_iterator end_iter;
	for (fs::directory_iterator iter(path); iter != end_iter; ++iter)
	{
		if (fs::is_regular_file(iter->status()))
		{
			filenames.push_back(iter->path().string());
		}

		if (fs::is_directory(iter->status()))
		{
			get_filenames(iter->path().string(), filenames);
		}
	}

	return filenames.size();
}

Application::Application(int argc, char* argv[])
{
	GDALAllRegister();

	// Check command line arguments.

	address_ = net::ip::make_address(argv[1]);
	port_ = static_cast<unsigned short>(std::atoi(argv[2]));
	doc_root_ = std::make_shared<std::string>(argv[3]);
	threads_ = std::max<int>(1, std::atoi(argv[4]));

	char _szPath[MAX_PATH + 1] = { 0 };

	GetModuleFileName(NULL, _szPath, MAX_PATH);
	(strrchr(_szPath, '\\'))[1] = 0;

	for (int n = 0; _szPath[n]; n++)
	{
		if (_szPath[n] != ('\\'))
		{
			service_data_folder_ += _szPath[n];
		}
		else
		{
			service_data_folder_ += '\\';
		}
	}

	service_data_folder_ += "../service_data";
	get_filenames(service_data_folder_, service_files_);

	InitBandMap();
}

Application::~Application() 
{

}

void Application::InitBandMap()
{
	std::map<std::string, std::vector<int>> map_landsat8;
	map_landsat8.emplace("自然色", std::vector<int>{ 4, 3, 2 });
	map_landsat8.emplace("红外彩色", std::vector<int>{5, 4, 3});
	map_landsat8.emplace("健康植被", std::vector<int>{ 5, 6, 2 });
	map_landsat8.emplace("陆地/水体", std::vector<int>{ 5, 6, 4 });
	map_landsat8.emplace("植被/水体", std::vector<int>{ 5, 7, 1 });
	map_landsat8.emplace("裸露地表", std::vector<int>{ 6, 3, 2 });
	map_landsat8.emplace("农业", std::vector<int>{ 6, 5, 2 });
	map_landsat8.emplace("植被分析", std::vector<int>{ 6, 5, 4 });
	map_landsat8.emplace("森林火灾", std::vector<int>{ 7, 5, 2 });
	map_landsat8.emplace("大气去除", std::vector<int>{ 7, 5, 3 });
	map_landsat8.emplace("短波红外", std::vector<int>{ 7, 5, 4 });
	map_landsat8.emplace("假彩色-城市", std::vector<int>{ 7, 6, 4 });
	map_landsat8.emplace("大气渗透", std::vector<int>{ 7, 6, 5 });

	std::map<std::string, std::vector<int>> map_sentinel2;
	map_sentinel2.emplace("自然色", std::vector<int>{ 4, 3, 2 });
	map_sentinel2.emplace("红外彩色", std::vector<int>{ 8, 4, 3 });
	map_sentinel2.emplace("健康植被", std::vector<int>{ 8, 11, 2 });
	map_sentinel2.emplace("陆地/水体", std::vector<int>{ 8, 11, 4 });
	map_sentinel2.emplace("植被/水体", std::vector<int>{ 8, 12, 1 });
	map_sentinel2.emplace("裸露地表", std::vector<int>{ 11, 3, 2 });
	map_sentinel2.emplace("农业", std::vector<int>{ 11, 8, 2 });
	map_sentinel2.emplace("植被分析", std::vector<int>{ 11, 8, 4 });
	map_sentinel2.emplace("森林火灾", std::vector<int>{ 12, 8, 2 });
	map_sentinel2.emplace("大气去除", std::vector<int>{ 12, 8, 3 });
	map_sentinel2.emplace("短波红外", std::vector<int>{ 12, 8, 4 });
	map_sentinel2.emplace("假彩色-城市", std::vector<int>{ 12, 11, 4 });
	map_sentinel2.emplace("大气渗透", std::vector<int>{ 12, 11, 8 });

	std::map<std::string, std::vector<int>> map_gf;
	map_gf.emplace("自然色", std::vector<int>{ 3, 2, 1 });
	map_gf.emplace("红外彩色", std::vector<int>{ 4, 3, 2 });

	band_map_description_.emplace("Landsat8", map_landsat8);
	band_map_description_.emplace("Sentinel2", map_sentinel2);
	band_map_description_.emplace("高分系列", map_gf);
}

void Application::Run()
{
	// The io_context is required for all I/O
	net::io_context ioc{ threads_ };

	// Create and launch a listening port
	std::make_shared<Listener>(
		ioc,
		tcp::endpoint{ address_, port_ },
		doc_root_)->run();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads_ - 1);
	for (auto i = threads_ - 1; i > 0; --i)
		v.emplace_back(
			[&ioc]
			{
				ioc.run();
			});
	ioc.run();
}