#include "handler.h"
#include <string>
#include "application.h"

void Handler::QueryDataPath(const Url& url, std::list<std::string>& paths)
{
	//paths.emplace_back("D:\\work\\data\\GF1_PMS2_E113.9_N34.4_20181125_L5A_0003622463.tiff");
	//paths.emplace_back("d:/soa_share/GF1_PMS1_E110.4_N34.7_20170513_L1A0003920001-MSS1.tiff");

	std::string path;
	if (url.QueryValue("path", path))
	{
		paths.emplace_back(path);
	}
	else
	{
		paths = global_app->service_files();
	}
}

std::string Handler::QueryStyle(const Url& url)
{
	std::string style = "";
	url.QueryValue("style", style);
	return style;
}

int Handler::QuerySRS(const Url& url)
{
	int epsg = -1;

	std::string str_epsg;
	if (url.QueryValue("srs", str_epsg))
	{
		epsg = atoi(str_epsg.c_str());
	}

	return epsg;
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

	if (url.QueryValue("tilecol", value_x))
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

	if (url.QueryValue("tilerow", value_x))
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

	if (url.QueryValue("tilematrix", value_x))
	{
		nx = atoi(value_x.c_str());
		return nx;
	}

	return nx;
}

int Handler::QueryTileWidth(const Url& url)
{
	int width = 256;
	std::string value;

	if (url.QueryValue("width", value))
	{
		width = atoi(value.c_str());
		return width;
	}

	return width;
}

int Handler::QueryTileHeight(const Url& url)
{
	int height = 256;
	std::string value;

	if (url.QueryValue("height", value))
	{
		height = atoi(value.c_str());
		return height;
	}

	return height;
}