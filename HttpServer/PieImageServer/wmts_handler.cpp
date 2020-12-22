#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "resource_pool.h"
#include "storage_manager.h"

bool WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, request_body, result);
		}
		else if (request.compare("UpdateDataStyle") == 0)
		{
			return UpdateDataStyle(request_body, result);
		}
		else if (request.compare("ClearAllDatasets") == 0)
		{
			return ClearAllDatasets(request_body, result);
		}
		else if (request.compare("AddImages") == 0)
		{
			return AddImages(request_body, result);
		}
		else if (request.compare("GetImages") == 0)
		{
			return GetImages(request_body, result);
		}
		else if (request.compare("ClearImages") == 0)
		{
			return ClearImages(request_body, result);
		}

		return false;
	}

	return GetTile(doc_root, url, request_body, result);
}

bool WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	int epsg_code = QuerySRS(url);
	if (epsg_code == -1)
	{
		//默认 web_mercator
		epsg_code = 3857;
	}

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env, epsg_code);

	std::list<std::pair<DatasetPtr, StylePtr>> datasets;

	//先判断url里有没有“key”
	std::string md5;
	std::string data_style_json;
	if (url.QueryValue("key", md5))
	{
		if (!StorageManager::GetDataStyle(md5, data_style_json))
			return false;
	}
	else if (!request_body.empty())
	{
		data_style_json = request_body;
	}
	else if (!url.QueryValue("info", data_style_json))
	{
		return false;
	}

	std::list<std::pair<std::string, std::string>> data_info;
	QueryDataInfo(data_style_json, data_info);

	for (auto info : data_info)
	{
		const std::string& path = info.first;
		std::string style_string = info.second;

		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(path));
		if (tiffDataset == nullptr)
			continue;

		StylePtr style_clone = StyleManager::GetStyle(style_string, tiffDataset);
		style_clone->set_code(epsg_code);
		datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
	}

	return WMSHandler::GetRenderBytes(datasets, env, 256, 256, result);
}