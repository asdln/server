#pragma once

#include "handler.h"

class WMSHandler : public Handler
{
public:

	virtual bool Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result) override;

	bool GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool UpdateDataStyle(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetStyle(const Url& url, std::shared_ptr<HandleResult> result);

	bool UpdateStyle(const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool ClearAllDatasets(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetLayInfo(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetImageInfo(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//返回group的经纬度范围
	bool GetGroupEnvelope(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//返回经纬度范围
	bool GetEnvlope(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"group":"group1", "images":["c:/1.tif", "c:/2.tif", "c:/3.tif"]}
	bool AddImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool RemoveImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool RemoveGroups(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool AddGroups(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetGroups(std::shared_ptr<HandleResult> result);

	//{"group":"group1"}
	bool GetImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool SetImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	//{"group":"group1"}
	bool ClearImages(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool GetGroupCacheState(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool SetGroupCacheState(const std::string& request_body, std::shared_ptr<HandleResult> result);

	bool ClearGroupCache(const std::string& request_body, std::shared_ptr<HandleResult> result);

protected:

	bool GetDatasets(int epsg_code, const std::string& data_style_json, bool one_to_one
		, const std::list<std::string>& data_paths, std::list<std::pair<DatasetPtr, StylePtr>>& datasets, Format& format);

	bool GetDataStyleString(const Url& url, const std::string& request_body, std::string& data_style_json);

	bool GetStyleString(const Url& url, const std::string& request_body, std::string& style_json
		, std::list<std::string>& paths, Format& format, std::string& layerID);

	bool GetHandleResult(bool use_cache, Envelop env, int tile_width, int tile_height, int epsg_code
		, const std::string& data_style, const std::string& amazon_md5, bool one_to_one, 
		const std::list<std::string>& data_paths, Format format, std::shared_ptr<HandleResult> result);

protected:

	static unsigned char cache_tag_;
};