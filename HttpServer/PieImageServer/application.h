#pragma once

#include <memory>
#include <string>
#include <boost/asio/ip/address.hpp>
#include <list>
#include <vector>

class Application
{
public:

	Application(int argc, char* argv[]);
	virtual ~Application();

	void Run();

	std::list<std::string>& service_files() { return service_files_; }

	int StatisticSize() { return statistic_window_size_; }

protected:

	void InitBandMap();

	//bool LoadConfig();

protected:

	boost::asio::ip::address address_;
	unsigned short port_;
	std::shared_ptr<std::string> doc_root_;
	int threads_;

	std::string gdal_cache_size_ = "1000";

	int statistic_window_size_;

	std::string app_path_;

	std::string service_data_path_ = "";

	std::list<std::string> service_files_;

	std::map<std::string, std::map<std::string, std::vector<int>>> band_map_description_;
};

typedef std::shared_ptr<Application> ApplicationPtr;
extern ApplicationPtr global_app;