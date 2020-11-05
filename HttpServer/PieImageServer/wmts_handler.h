#ifndef HTTPSERVER_WMTS_HANDLER_H_
#define HTTPSERVER_WMTS_HANDLER_H_


#include "wms_handler.h"

class WMTSHandler : public WMSHandler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);
	
	bool GetCapabilities(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);
	
	bool GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, std::shared_ptr<HandleResult> result);
	
	bool UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:


};

#endif //HTTPSERVER_WMTS_HANDLER_H_