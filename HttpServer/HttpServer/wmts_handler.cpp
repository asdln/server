#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"

std::shared_ptr<HandleResult> WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, const std::string& mimeType)
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
		else if (request.compare("UpdateStyle") == 0)
		{
			return UpdateStyle(doc_root, url, request_body, mimeType);
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

	std::string style_str = QueryStyle(url);
	std::shared_ptr<Style> style = StyleManager::GetStyle(style_str);

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env);

	std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	bool bRes = pRequestProcessor->GetTileData(paths, env, 256, &pData, nDataSize, style.get(), mimeType);
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

std::shared_ptr<HandleResult> WMTSHandler::UpdateStyle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, const std::string& mimeType)
{
	StyleManager::UpdateStyle(request_body);

	return nullptr;
}