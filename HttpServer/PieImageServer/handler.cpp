#include "handler.h"
#include <string>
#include "application.h"
#include "style.h"
#include "style_manager.h"
#include "CJsonObject.hpp"

void Handler::QueryDataPath(const Url& url, std::list<std::string>& paths)
{
	//paths.emplace_back("D:\\work\\data\\GF1_PMS2_E113.9_N34.4_20181125_L5A_0003622463.tiff");
	//paths.emplace_back("d:/soa_share/GF1_PMS1_E110.4_N34.7_20170513_L1A0003920001-MSS1.tiff");

	std::string path;
	if (url.QueryValue("path", path))
	{
		std::vector<std::string> path_array;
		Split(path, path_array, ";");
		for (int i = 0; i < path_array.size(); i++)
		{
			paths.emplace_back(path_array[i]);
		}
	}
	else
	{
		paths = global_app->service_files();
	}
}

void Handler::QueryDataInfo(const std::string& request_body, std::list<std::pair<std::string, std::string>>& data_info)
{
	neb::CJsonObject oJson_info;
	neb::CJsonObject oJson(request_body);
	if (oJson.Get("info", oJson_info))
	{
		if (oJson_info.IsArray())
		{
			int array_size = oJson_info.GetArraySize();
			for (int i = 0; i < array_size; i++)
			{
				neb::CJsonObject oJson_dataset_info = oJson_info[i];
				std::string path = "";
				std::string style_string = "";
				neb::CJsonObject oJson_style;
				if (oJson_dataset_info.Get("path", path))
				{
					//style_string ©иртн╙©у
					if (oJson_dataset_info.Get("style", oJson_style))
					{
						style_string = oJson_style.ToString();
					}

					data_info.emplace_back(std::make_pair(path, style_string));
				}
			}
		}
	}
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