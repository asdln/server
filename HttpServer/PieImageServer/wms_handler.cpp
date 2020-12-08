#include "wms_handler.h"
#include "style_manager.h"
#include "tiff_dataset.h"
#include "resource_pool.h"

bool WMSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetMap") == 0)
		{
			return GetMap(doc_root, url, request_body, result);
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

	return GetMap(doc_root, url, request_body, result);
}

bool WMSHandler::GetRenderBytes(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height, std::shared_ptr<HandleResult> result)
{
	BufferPtr buffer = TileProcessor::GetCombinedData(datasets, env, tile_width, tile_height);

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

bool WMSHandler::GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::list<std::string> paths;
	QueryDataPath(url, request_body, paths);

	//暂时只获取第一个数据集
	std::string filePath = paths.front();
	std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(filePath));
	if (tiffDataset == nullptr)
		return false;

	StylePtr style_clone = GetStyle(url, request_body, tiffDataset);

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

	//如果在url里显式的指定投影，则覆盖
	int epsg_code = QuerySRS(url);
	if (epsg_code != -1)
	{
		style_clone->set_code(epsg_code);
	}

	double no_data_value = 0.;
	bool have_no_data = QueryNoDataValue(url, no_data_value);

	if (have_no_data)
	{
		style_clone->GetStretch()->SetUseExternalNoDataValue(have_no_data);
		style_clone->GetStretch()->SetExternalNoDataValue(no_data_value);
	}

	std::list<std::pair<DatasetPtr, StylePtr>> datasets;
	datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
	return GetRenderBytes(datasets, env, tile_width, tile_height, result);
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