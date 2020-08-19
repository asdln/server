#include "wmts_handler.h"
#include "jpg_buffer.h"

std::shared_ptr<HandleResult> WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, mimeType);
		}
		else if (request.compare("GetCapabilities") == 0)
		{
			return GetCapabilities(doc_root, url, mimeType);
		}
		else if (request.compare("GetFeatureInfo") == 0)
		{
			return GetFeatureInfo(doc_root, url, mimeType);
		}
	}

	return GetTile(doc_root, url, mimeType);
}

std::shared_ptr<HandleResult> WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType)
{
	void* pData = nullptr;
	unsigned long nDataSize = 0;

	std::list<std::string> paths;
	QueryDataPath(url, paths);

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env);

	std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	bool bRes = pRequestProcessor->GetTileData(paths, env, 256, &pData, nDataSize, mimeType);
	auto result = std::make_shared<HandleResult>();

	if (bRes)
	{
		auto buffer = std::make_shared<JpgBuffer>();
		buffer->set_data(pData, nDataSize);
		result->set_buffer(buffer);
	}

	return result;
}

std::shared_ptr<HandleResult> WMTSHandler::GetCapabilities(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType)
{
	return nullptr;
}

std::shared_ptr<HandleResult> WMTSHandler::GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType)
{
	return nullptr;
}