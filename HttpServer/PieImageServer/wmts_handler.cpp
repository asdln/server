#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "resource_pool.h"

bool WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, request_body, result);
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

	return GetTile(doc_root, url, request_body, result);
}

bool WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::list<std::string> paths;
	QueryDataPath(url, request_body, paths);

	int epsg_code = QuerySRS(url);
	if (epsg_code == -1)
	{
		//д╛хо web_mercator
		epsg_code = 3857;
	}

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env, epsg_code);

	double no_data_value = 0.;
	bool have_no_data = QueryNoDataValue(url, no_data_value);

	std::list<std::pair<DatasetPtr, StylePtr>> datasets;
	for (auto path : paths)
	{
		std::string filePath = path;
		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(filePath));
		if (tiffDataset == nullptr)
			return false;

		StylePtr style_clone = GetStyle(url, request_body, tiffDataset);
		style_clone->set_code(epsg_code);

		if (have_no_data)
		{
			style_clone->GetStretch()->SetUseExternalNoDataValue(have_no_data);
			style_clone->GetStretch()->SetExternalNoDataValue(no_data_value);
		}

		datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
	}

	return WMSHandler::GetRenderBytes(datasets, env, 256, 256, result);
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