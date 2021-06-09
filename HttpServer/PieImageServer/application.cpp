#include "application.h"

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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include "application.h"
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "gdal_priv.h"
#include "listener.h"
#include <fstream>
#include "etcd_storage.h"
#include "resource_pool.h"
#include "argparse.hpp"
#include "amazon_s3.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

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
			std::string file_path = iter->path().string();
			std::string suffix_str = file_path.substr(file_path.find_last_of('.') + 1);
			if(suffix_str.compare("tiff") == 0|| suffix_str.compare("TIFF") == 0
				|| suffix_str.compare("tif") == 0 || suffix_str.compare("TIF") == 0
				|| suffix_str.compare("img") == 0 || suffix_str.compare("IMG") == 0)
			
			filenames.push_back(file_path);
		}

		if (fs::is_directory(iter->status()))
		{
			get_filenames(iter->path().string(), filenames);
		}
	}

	return filenames.size();
}

#define MAX_PATH_LEN   256

#ifdef __GNUC__
int get_process_path(char *process_path){

    char *p = NULL;
    int n = 0;

    memset(process_path,0x00,MAX_PATH_LEN);

    n = readlink("/proc/self/exe",process_path,MAX_PATH_LEN);

    if (NULL != (p = strrchr(process_path,'/'))){

        *p = '\0';
    }else{

        printf("wrong process path");
        return -1;
    }

    return 0;
}
#endif

Application::Application(int argc, char* argv[])
{
	argparse::ArgumentParser program("server");

	program.add_argument("--port")
		.help("port listened on")
		.default_value(8083)
		.action([](const std::string& value) { return std::stoi(value); });

	program.add_argument("--thread")
		.help("number of thread created")
		.default_value(12)
		.action([](const std::string& value) { return std::stoi(value); });

	program.add_argument("--statistic_size")
		.help("histogram statistic size")
		.default_value(1024)
		.action([](const std::string& value) { return std::stoi(value); });

	program.add_argument("--gdal_cache_size")
		.help("gdal maximum cache size")
		.default_value(std::string("1000"));

	program.add_argument("--use_amazon_s3")
		.help("use amazon s3 as image cache")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("--amazon_s3_bucket_name")
		.help("amazon s3 bucket name for caching")
		.default_value(std::string(""));

    program.add_argument("--use_etcd_v2")
        .help("is use etcd_v2")
		.default_value(false)
		.implicit_value(true);

    program.add_argument("--use_etcd_v3")
        .help("is use etcd_v3")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--etcd_v2_host")
        .help("etcd_v2 service's host")
		.default_value(std::string("127.0.0.1"));

    program.add_argument("--etcd_v2_port")
        .help("etcd_v2 service's port")
		.default_value(std::string("2379"));

    program.add_argument("--etcd_v3_address")
        .help("etcd_v3 service's address, cluster address example:\"http://a.com:2379;http://b.com:2379;http://c.com:2379\"")
        .default_value(std::string("http://127.0.0.1:2379"));

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cout << "ERROR: " << std::endl << err.what() << std::endl;
		std::cout << "argument parse error: " << err.what() << std::endl;
		std::cout << "please input correct argument, program exited " << std::endl;
		std::cout << program;
		exit(0);
	}

	port_ = program.get<int>("--port");
	threads_ = program.get<int>("--thread");
	statistic_window_size_ = program.get<int>("--statistic_size");
	gdal_cache_size_ = program.get<std::string>("--gdal_cache_size");

    EtcdStorage::use_etcd_v2_ = program.get<bool>("--use_etcd_v2");

    EtcdStorage::host_v2_ = program.get<std::string>("--etcd_v2_host");
    EtcdStorage::port_v2_ = program.get<std::string>("--etcd_v2_port");

	bool use_s3 = program.get<bool>("--use_amazon_s3");
	AmazonS3::SetUseS3(use_s3);
	std::string bucket_name;
	if (use_s3)
	{
		bucket_name = program.get<std::string>("--amazon_s3_bucket_name");
		AmazonS3::SetBucketName(bucket_name);

		if (bucket_name.empty())
		{
			LOG(ERROR) << "amazon s3 bucket name is empty, use --amazon_s3_bucket_name flag";
			exit(0);
		}

		AmazonS3::init();

		if (!AmazonS3::CreateBucket())
		{
			exit(0);
		}
	}

#ifndef ETCD_V2

    EtcdStorage::use_etcd_v3_ = program.get<bool>("--use_etcd_v3");
    EtcdStorage::address_v3_ = program.get<std::string>("--etcd_v3_address");

#endif

	std::cout << program << std::endl << std::endl;

    if(EtcdStorage::use_etcd_v2_ && EtcdStorage::use_etcd_v3_)
    {
        EtcdStorage::use_etcd_v3_ = false;
    }
	

	GDALAllRegister();

    const int max_path = MAX_PATH_LEN;
    char _szPath[max_path + 1] = { 0 };

#ifdef __GNUC__

    get_process_path(_szPath);
    app_path_ = _szPath + std::string("/");

#else

    GetModuleFileName(NULL, _szPath, max_path);
	(strrchr(_szPath, '\\'))[1] = 0;

	for (int n = 0; _szPath[n]; n++)
	{
		if (_szPath[n] != ('\\'))
		{
			app_path_ += _szPath[n];
		}
		else
		{
			app_path_ += '\\';
		}
	}

#endif

    std::string gdal_data_path = app_path_ + "data";
    CPLSetConfigOption("GDAL_DATA", gdal_data_path.c_str());

	//CPLSetConfigOption("CPL_DEBUG", "ON");

	LOG(INFO) << "GDAL_DATA : " << gdal_data_path;

	//if (service_files_.empty())
	//{
	//	service_data_path_ = app_path_ + "../service_data";

	//	LOG(INFO) << "service_data_path_ : " << service_data_path_;
	//	get_filenames(service_data_path_, service_files_);
	//}

	//for (auto& file_name : service_files_)
	//{
	//	LOG(INFO) << "data: " << file_name;
	//}

    address_ = net::ip::make_address("0.0.0.0");
	doc_root_ = std::make_shared<std::string>("tile");

	CPLSetConfigOption("GDAL_CACHEMAX", gdal_cache_size_.c_str());
	LOG(INFO) << "GDAL Version:  " << GDALVersionInfo("--version");
	LOG(INFO) << "gdal_cache_size:  " << gdal_cache_size_ << "Mb";

	//LoadConfig();

	ResourcePool::GetInstance()->SetDatasetPoolMaxCount(threads_ + 1);

	LOG(INFO) << "thread : " << threads_ << "   port : " << port_ << "    statistic_size: " << statistic_window_size_;

    if(EtcdStorage::use_etcd_v2_)
    {
        LOG(INFO) << "use_etcd_v2 : " << EtcdStorage::use_etcd_v2_ << std::endl;
        LOG(INFO) << "etcd_v2_host : " << EtcdStorage::host_v2_ << "    etcd_v2_port : " << EtcdStorage::port_v2_;
    }
    else if(EtcdStorage::use_etcd_v3_)
    {
        LOG(INFO) << "use_etcd_v3 : " << EtcdStorage::use_etcd_v3_;
        LOG(INFO) << "etcd_v3_address : " << EtcdStorage::address_v3_;
    }
    else
    {
        LOG(INFO) << "etcd is not used" << std::endl;
    }

	EtcdStorage etcd_storage;
	if (etcd_storage.IsUseEtcd())
    {
		LOG(INFO) << "try to connect to etcd, please wait...";
        if (!etcd_storage.SetValue("pie", "test"))
		{
			LOG(INFO) << "etcd connect test failed!";
		}
		else
		{
            std::string value;
            etcd_storage.GetValue("pie", value);
            if(value.compare("test") == 0)
                LOG(INFO) << "etcd connect test successful!";
            else
                LOG(INFO) << "etcd get value test failed!";
		}
	}

	LOG(INFO) << "use_amazon_s3 : " << use_s3;
	if (use_s3)
	{
		LOG(INFO) << "amazon s3 bucket name: " << bucket_name;
	}

	InitBandMap();
	LOG(INFO) << "server started";
}

Application::~Application() 
{

}

//bool Application::LoadConfig()
//{
//	threads_ = 12;
//	port_ = 8083;
//	statistic_window_size_ = 1024;
//	EtcdStorage::port_ = "2379";
//	EtcdStorage::host_ = "0.0.0.0";
//
//	if (!boost::filesystem::exists(app_path_ + "config.ini")) 
//	{
//		LOG(INFO) << "config.ini not exists : " << app_path_ + "config.ini";
//		return false;
//	}
//
//	boost::property_tree::ptree root_node, tag_system;
//	boost::property_tree::ini_parser::read_ini(app_path_ + "config.ini", root_node);
//	tag_system = root_node.get_child("Server");
//	if (tag_system.count("port") == 1) 
//	{
//		port_ = tag_system.get<int>("port");
//	}
//	
//	if (tag_system.count("threads") == 1) 
//	{
//		threads_ = std::max<int>(1, tag_system.get<int>("threads"));
//	}
//
//	if (tag_system.count("statistic_size") == 1)
//	{
//		statistic_window_size_ = std::max<int>(1, tag_system.get<int>("statistic_size"));
//	}
//
//	if (tag_system.count("gdal_cache_size") == 1)
//	{
//		gdal_cache_size_ = tag_system.get<std::string>("gdal_cache_size");
//	}
//
//	boost::property_tree::ptree tag_etcd = root_node.get_child("Etcd");
//	if (tag_etcd.count("port") == 1) 
//	{
//		EtcdStorage::port_ = tag_etcd.get<std::string>("port");
//	}
//
//	if (tag_etcd.count("host") == 1) 
//	{
//		EtcdStorage::host_ = tag_etcd.get<std::string>("host");
//	}
//
//	return true;
//}

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
