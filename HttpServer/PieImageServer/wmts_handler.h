#pragma once


#include "wms_handler.h"

class WMTSHandler : public WMSHandler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:


};