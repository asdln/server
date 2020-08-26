#ifndef HTTPSERVER_HTTP_HANDLER_H_
#define HTTPSERVER_HTTP_HANDLER_H_


#include "handler.h"
#include "url.h"

class HttpHandler : public Handler
{
public:
	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, const std::string& mimeType, std::shared_ptr<HandleResult> result) override;

};

#endif //HTTPSERVER_HTTP_HANDLER_H_