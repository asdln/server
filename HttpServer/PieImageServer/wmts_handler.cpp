#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"

bool WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, result);
		}
		else if (request.compare("GetCapabilities") == 0)
		{
			return GetCapabilities(doc_root, url, result);
		}
		else if (request.compare("GetFeatureInfo") == 0)
		{
			return GetFeatureInfo(doc_root, url, result);
		}
		else if (request.compare("UpdateStyle") == 0)
		{
			return UpdateStyle(request_body, result);
		}
	}

	return GetTile(doc_root, url, result);
}

bool WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
{
	std::list<std::string> paths;
	QueryDataPath(url, paths);

	double no_data_value = 0.;
	bool have_no_data = QueryNoDataValue(url, no_data_value);

	std::vector<std::string> tokens;
	std::string style_str = QueryStyle(url);
	Split(style_str, tokens, ":");

	StylePtr style;
	if (tokens.size() == 3)
	{
		style = StyleManager::GetStyle(tokens[0] + ":" + tokens[1], atoi(tokens[2].c_str()));
	}

	if (style == nullptr)
		style = std::make_shared<Style>();

	//clone一份，防止后面修改srs_epsg_code_时多线程产生冲突
	Style style_clone = *style;

	//如果在url里显式的指定投影，则覆盖
	int epsg_code = QuerySRS(url);
	if (epsg_code != -1)
	{
		style_clone.srs_epsg_code_ = epsg_code;
	}

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env, style_clone.srs_epsg_code_);

	if (have_no_data)
	{
		style_clone.stretch_->SetUseExternalNoDataValue(have_no_data);
		style_clone.stretch_->SetExternalNoDataValue(no_data_value);
	}

	return WMSHandler::GetTileData(paths, env, &style_clone, 256, 256, result);
}

bool WMTSHandler::GetCapabilities(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
{
	return true;
}

bool WMTSHandler::GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
{
	return true;
}

bool WMTSHandler::UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string style_key;

	if (!StyleManager::UpdateStyle(request_body, style_key))
		return false;

	auto string_body = std::make_shared<http::response<http::string_body>>(http::status::ok, result->version());
	string_body->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	string_body->set(http::field::content_type, "text/html");
	string_body->keep_alive(result->keep_alive());
	string_body->body() = style_key;
	string_body->prepare_payload();

	string_body->set(http::field::access_control_allow_origin, "*");
	string_body->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
	string_body->set(http::field::access_control_allow_credentials, "true");

	result->set_string_body(string_body);

	return true;
}