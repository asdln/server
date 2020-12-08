#ifndef PIEIMAGESERVER_WMS_HANDLER_H_
#define PIEIMAGESERVER_WMS_HANDLER_H_

#include "handler.h"

class WMSHandler : public Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetRenderBytes(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height, std::shared_ptr<HandleResult> result);

	bool GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetCapabilities(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);

	bool GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);

	bool UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);
};

#endif //PIEIMAGESERVER_WMS_HANDLER_H_