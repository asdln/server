#ifndef HTTPSERVER_HANDLER_H_
#define HTTPSERVER_HANDLER_H_

#include <boost/beast/http.hpp>
#include "session.h"
#include "handle_result.h"
#include "style.h"
#include "dataset.h"

class Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) { return false; }

	virtual int QueryX(const Url& url);
	virtual int QueryY(const Url& url);
	virtual int QueryZ(const Url& url);

	virtual int QueryTileWidth(const Url& url);
	virtual int QueryTileHeight(const Url& url);

	virtual void QueryDataPath(const Url& url, const std::string& request_body, std::list<std::string>& paths);

	virtual bool QueryNoDataValue(const Url& url, double& value);

	virtual StylePtr GetStyle(const Url& url, const std::string& request_body, DatasetPtr dataset);

	virtual int QuerySRS(const Url& url);
};

#endif //HTTPSERVER_HANDLER_H_