#include "handler.h"
#include <string>
#include "application.h"

void Handler::QueryDataPath(const Url& url, std::list<std::string>& paths)
{
	//paths.emplace_back("D:\\work\\data\\GF1_PMS2_E113.9_N34.4_20181125_L5A_0003622463.tiff");
	//paths.emplace_back("d:/soa_share/GF1_PMS1_E110.4_N34.7_20170513_L1A0003920001-MSS1.tiff");
	
	paths = global_app->service_files();
}

std::string Handler::QueryStyle(const Url& url)
{
	std::string style = "";
	if (!url.QueryValue("style", style))
	{
		url.QueryValue("STYLE", style);
	}

	return style;
}

int Handler::QueryX(const Url& url)
{
	int nx = 0;
	std::string value_x;
	if (url.QueryValue("x", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("X", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("TILECOL", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	return nx;
}

int Handler::QueryY(const Url& url)
{
	int nx = 0;
	std::string value_x;
	if (url.QueryValue("y", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("Y", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("TILEROW", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	return nx;
}

int Handler::QueryZ(const Url& url)
{
	int nx = 0;
	std::string value_x;
	if (url.QueryValue("z", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("Z", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	if (url.QueryValue("TILEMATRIX", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	return nx;
}