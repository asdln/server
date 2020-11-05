#include "handler.h"
#include <string>
#include "application.h"
#include "style.h"
#include "style_manager.h"

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

StylePtr Handler::GetStyle(const Url& url, const std::string& request_body)
{
	//如果从body里获取到了style信息，则优先body创建
	StylePtr style = StyleManager::FromJson(request_body);
	if (style == nullptr)
	{
		std::vector<std::string> tokens;
		std::string style_str = QueryStyle(url);
		Split(style_str, tokens, ":");

		if (tokens.size() == 3)
		{
			style = StyleManager::GetStyle(tokens[0] + ":" + tokens[1], atoi(tokens[2].c_str()));
		}

		if (style == nullptr)
			style = std::make_shared<Style>();
	}

	return style;
}

bool Handler::QueryNoDataValue(const Url& url, double& value)
{
	std::string no_data_value;
	if (url.QueryValue("nodatavalue", no_data_value))
	{
		value = atoi(no_data_value.c_str());
		return true;
	}
	else
	{
		return false;
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