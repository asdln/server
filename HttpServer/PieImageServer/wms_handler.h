#ifndef PIEIMAGESERVER_WMS_HANDLER_H_
#define PIEIMAGESERVER_WMS_HANDLER_H_

#include "handler.h"

class WMSHandler : public Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetTileData(std::list<std::string> paths, const Envelop& env, Style* style, int tile_width, int tile_height, std::shared_ptr<HandleResult> result);

	bool GetMap(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);

	bool GetCapabilities(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);

	bool GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);

	bool UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);
};

#endif //PIEIMAGESERVER_WMS_HANDLER_H_