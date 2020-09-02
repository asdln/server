#ifndef HTTPSERVER_WMTS_HANDLER_H_
#define HTTPSERVER_WMTS_HANDLER_H_


#include "handler.h"

class WMTSHandler : public Handler
{
public:

	//virtual void Handle(Session& ses, boost::beast::string_view doc_root, boost::beast::http::request<boost::beast::http::string_body>&& req) override;

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, const std::string& mimeType, std::shared_ptr<HandleResult> result) override;

	bool GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType, std::shared_ptr<HandleResult> result);
	
	bool GetCapabilities(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType, std::shared_ptr<HandleResult> result);
	
	bool GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType, std::shared_ptr<HandleResult> result);
	
	bool UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:


};

#endif //HTTPSERVER_WMTS_HANDLER_H_