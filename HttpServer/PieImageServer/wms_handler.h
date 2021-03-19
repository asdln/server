#ifndef PIEIMAGESERVER_WMS_HANDLER_H_
#define PIEIMAGESERVER_WMS_HANDLER_H_

#include "handler.h"

class WMSHandler : public Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	//bool GetRenderBytes(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height, std::shared_ptr<HandleResult> result);

	bool GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool UpdateDataStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool ClearAllDatasets(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetLayInfo(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"user":"admin", "group":"group1", "images":["c:/1.tif", "c:/2.tif", "c:/3.tif"]}
	bool AddImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"user":"admin", "group":"group1"}
	bool GetImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"user":"admin", "group":"group1"}
	bool ClearImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:

	//bool GetDatasets(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::list<std::pair<DatasetPtr, StylePtr>>& datasets);
	bool GetDatasets(int epsg_code, const std::string& data_style_json, std::list<std::pair<DatasetPtr, StylePtr>>& datasets);

	bool GetDataStyleString(const Url& url, const std::string& request_body, std::string& data_style_json);

	bool GetHandleResult(bool use_cache, Envelop env, int tile_width, int tile_height, int epsg_code, const std::string& data_style, const std::string& amazon_md5, std::shared_ptr<HandleResult> result);
};

#endif //PIEIMAGESERVER_WMS_HANDLER_H_