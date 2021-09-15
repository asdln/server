#pragma once


#include "wms_handler.h"

class WMTSHandler : public WMSHandler
{
public:

	virtual bool Handle(const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetTile(const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:


};