#pragma once

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

	virtual int QuerySRS(const Url& url);

	virtual bool QueryIsUseCache(const Url& url);

	std::shared_ptr<http::response<http::string_body>> CreateStringResponse(http::status status_code, int version, bool keep_alive, const std::string& res_string);
};