#ifndef HTTPSERVER_HANDLER_H_
#define HTTPSERVER_HANDLER_H_

#include <boost/beast/http.hpp>
#include "session.h"
#include "handle_result.h"

class Handler
{
public:

	virtual std::shared_ptr<HandleResult> Handle(boost::beast::string_view doc_root, const Url& url, const std::string& mimeType) { return nullptr; }

	virtual int QueryX(const Url& url);
	virtual int QueryY(const Url& url);
	virtual int QueryZ(const Url& url);

	virtual void QueryDataPath(const Url& url, std::list<std::string>& paths);
};

#endif //HTTPSERVER_HANDLER_H_