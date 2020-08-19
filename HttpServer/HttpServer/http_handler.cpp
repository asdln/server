#include "http_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"

std::shared_ptr<HandleResult> HttpHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType)
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
