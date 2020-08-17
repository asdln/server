#ifndef HTTPSERVER_HANDLER_H_
#define HTTPSERVER_HANDLER_H_

#include <boost/beast/http.hpp>
#include "session.h"

class Handler
{
public:

	//virtual void Handle(boost::beast::http::request<boost::beast::http::string_body>&& request) = 0;

	//virtual void Handle(boost::beast::string_view doc_root, boost::beast::http::request<boost::beast::http::string_body>&& request, session::send_lambda&& send);
};

#endif //HTTPSERVER_HANDLER_H_