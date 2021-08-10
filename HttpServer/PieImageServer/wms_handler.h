#pragma once

#include "handler.h"

class WMSHandler : public Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	//bool GetRenderBytes(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height, std::shared_ptr<HandleResult> result);

	bool GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool UpdateDataStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool UpdateStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool ClearAllDatasets(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetLayInfo(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetImageInfo(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//���ؾ�γ�ȷ�Χ
	bool GetEnvlope(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"group":"group1", "images":["c:/1.tif", "c:/2.tif", "c:/3.tif"]}
	bool AddImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool RemoveImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetGroups(std::shared_ptr<HandleResult> result);

	//{"group":"group1"}
	bool GetImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool SetImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"group":"group1"}
	bool ClearImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:

	//bool GetDatasets(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::list<std::pair<DatasetPtr, StylePtr>>& datasets);
	bool GetDatasets(int epsg_code, const std::string& data_style_json, bool one_to_one
		, const std::list<std::string>& data_paths, std::list<std::pair<DatasetPtr, StylePtr>>& datasets, Format& format, std::string cachekey);

	bool GetDataStyleString(const Url& url, const std::string& request_body, std::string& data_style_json);

	bool GetStyleString(const Url& url, const std::string& request_body, std::string& style_json
		, std::list<std::string>& paths, std::string& cachekey, Format& format);

	bool GetHandleResult(bool use_cache, Envelop env, int tile_width, int tile_height, int epsg_code
		, const std::string& data_style, const std::string& amazon_md5, bool one_to_one, 
		const std::list<std::string>& data_paths, std::string cachekey, Format format, std::shared_ptr<HandleResult> result);
};