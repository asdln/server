#include "handler.h"
#include <string>
#include "application.h"
#include "style.h"
#include "style_manager.h"

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

bool Handler::QueryIsUseCache(const Url& url)
{
	bool use_cache = true;

	std::string cache;
	if (url.QueryValue("cache", cache))
	{
		if (cache.compare("false") == 0 || cache.compare("0") == 0)
		{
			use_cache = false;
		}
	}

	return use_cache;
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

std::shared_ptr<http::response<http::string_body>> Handler::CreateStringResponse(http::status status_code, int version, bool keep_alive, const std::string& res_string)
{
	auto string_body = std::make_shared<http::response<http::string_body>>(status_code, version);
	string_body->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	string_body->set(http::field::content_type, "application/json");
	string_body->keep_alive(keep_alive);
	string_body->body() = res_string;
	string_body->prepare_payload();

	string_body->set(http::field::access_control_allow_origin, "*");
	string_body->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
	string_body->set(http::field::access_control_allow_credentials, "true");

	return string_body;
}