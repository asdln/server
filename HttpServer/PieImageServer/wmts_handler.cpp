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
		else if (request.compare("GetLayInfo") == 0)
		{
			return GetLayInfo(request_body, result);
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
		//д╛хо web_mercator
		epsg_code = 3857;
	}

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env, epsg_code);

	std::list<std::pair<DatasetPtr, StylePtr>> datasets;
	GetDatasets(doc_root, url, request_body, datasets);

	return WMSHandler::GetRenderBytes(datasets, env, 256, 256, result);
}