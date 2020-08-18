#ifndef HTTPSERVER_WMTS_HANDLER_H_
#define HTTPSERVER_WMTS_HANDLER_H_


#include "handler.h"

class WMTSHandler : public Handler
{
public:

	//virtual void Handle(boost::beast::http::request<boost::beast::http::string_body>&& request) override {}

	virtual void Handle(session& ses, boost::beast::string_view doc_root, boost::beast::http::request<boost::beast::http::string_body>&& req);

};

#endif //HTTPSERVER_WMTS_HANDLER_H_