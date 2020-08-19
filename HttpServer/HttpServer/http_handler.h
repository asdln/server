#ifndef HTTPSERVER_HTTP_HANDLER_H_
#define HTTPSERVER_HTTP_HANDLER_H_


#include "handler.h"
#include "url.h"

class HttpHandler : public Handler
{
public:
	virtual std::shared_ptr<HandleResult> Handle(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType);

};

#endif //HTTPSERVER_HTTP_HANDLER_H_