#include "wms_handler.h"
#include "style_manager.h"

bool WMSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetMap") == 0)
		{
			return GetMap(doc_root, url, result);
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

	return GetMap(doc_root, url, result);
}

bool WMSHandler::GetTileData(std::list<std::string> paths, const Envelop& env, Style* style, int tile_width, int tile_height, std::shared_ptr<HandleResult> result)
{
	std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	BufferPtr buffer = pRequestProcessor->GetTileData(paths, env, tile_width, tile_height, style);

	//if(buffer != nullptr)
	//{
	//	//test code
	//	FILE* pFile = nullptr;
	//	std::string path = "d:/test/";

	//	char string1[32];
	//	_itoa(nx, string1, 10);
	//	path += string1;
	//	path += "_";

	//	char string2[32];
	//	_itoa(ny, string2, 10);
	//	path += string2;
	//	path += "_";

	//	char string3[32];
	//	_itoa(nz, string3, 10);
	//	path += string3;

	//	path += ".jpg";

	//	fopen_s(&pFile, path.c_str(), "wb+");
	//	fwrite(buffer->data(), 1, buffer->size(), pFile);
	//	fclose(pFile);
	//	pFile = nullptr;
	//}

	if (buffer != nullptr)
	{
		result->set_buffer(buffer);

		http::buffer_body::value_type body;
		body.data = result->buffer()->data();
		body.size = result->buffer()->size();
		body.more = false;

		auto msg = std::make_shared<http::response<http::buffer_body>>(
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, result->version()));

		msg->set(http::field::server, BOOST_BEAST_VERSION_STRING);
		msg->set(http::field::content_type, "image/jpeg");

		msg->set(http::field::access_control_allow_origin, "*");
		msg->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
		msg->set(http::field::access_control_allow_credentials, "true");

		msg->content_length(result->buffer()->size());
		msg->keep_alive(result->keep_alive());

		result->set_buffer_body(msg);
	}

	return true;
}

bool WMSHandler::GetMap(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
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

	Envelop env;
	std::string env_string;
	if (url.QueryValue("bbox", env_string))
	{
		std::vector<std::string> tokens;
		Split(env_string, tokens, ",");
		if (tokens.size() != 4)
		{
			return false;
		}

		env.PutCoords(atof(tokens[0].c_str()), atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
	}
	else
	{
		return false;
	}

	int tile_width = QueryTileWidth(url);
	int tile_height = QueryTileHeight(url);

	std::string string_epsg;
	if (url.QueryValue("srs", string_epsg))
	{
		std::vector<std::string> tokens;
		Split(string_epsg, tokens, ":");
		if (tokens.size() == 2)
		{
			if (tokens[0].compare("epsg") == 0 || tokens[0].compare("EPSG") == 0)
			{
				int epsg = atoi(tokens[1].c_str());
				style_clone.srs_epsg_code_ = epsg;
			}
		}
	}

	if (have_no_data)
	{
		style_clone.stretch_->SetUseExternalNoDataValue(have_no_data);
		style_clone.stretch_->SetExternalNoDataValue(no_data_value);
	}

	return GetTileData(paths, env, &style_clone, tile_width, tile_height, result);
}

bool WMSHandler::GetCapabilities(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
{
	return true;
}

bool WMSHandler::GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result)
{
	return true;
}

bool WMSHandler::UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result)
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