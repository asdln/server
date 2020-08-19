#ifndef HTTPSERVER_WMTS_HANDLER_H_
#define HTTPSERVER_WMTS_HANDLER_H_


#include "handler.h"

class WMTSHandler : public Handler
{
public:

	//virtual void Handle(Session& ses, boost::beast::string_view doc_root, boost::beast::http::request<boost::beast::http::string_body>&& req) override;

	virtual std::shared_ptr<HandleResult> Handle(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType) override;

	std::shared_ptr<HandleResult> GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType);

	std::shared_ptr<HandleResult> GetCapabilities(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType);

	std::shared_ptr<HandleResult> GetFeatureInfo(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType);

protected:


};

#endif //HTTPSERVER_WMTS_HANDLER_H_