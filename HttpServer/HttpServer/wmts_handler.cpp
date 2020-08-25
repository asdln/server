#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"

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

	std::vector<std::string> tokens;
	std::string style_str = QueryStyle(url);
	Split(style_str, tokens, ":");

	if (tokens.size() != 2)
		return nullptr;

	std::shared_ptr<Style> style = StyleManager::GetStyle(tokens[0], atoi(tokens[1].c_str()));

	if (style == nullptr)
		return nullptr;

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
	std::string style_key;
	if (!StyleManager::UpdateStyle(request_body, style_key))
		return nullptr;



	return nullptr;
}