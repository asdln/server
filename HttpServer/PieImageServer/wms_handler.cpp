#include "wms_handler.h"
#include "style_manager.h"
#include "tiff_dataset.h"
#include "resource_pool.h"
#include "storage_manager.h"
#include "image_group_manager.h"
#include "amazon_s3.h"

bool WMSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetMap") == 0)
		{
			return GetMap(doc_root, url, request_body, result);
		}
		else if (request.compare("UpdateDataStyle") == 0)
		{
			return UpdateDataStyle(request_body, result);
		}
		else if (request.compare("ClearAllDatasets") == 0)
		{
			return ClearAllDatasets(request_body, result);
		}
		else if (request.compare("AddImages") == 0)
		{
			return AddImages(request_body, result);
		}
		else if (request.compare("GetImages") == 0)
		{
			return GetImages(request_body, result);
		}
		else if (request.compare("ClearImages") == 0)
		{
			return ClearImages(request_body, result);
		}
		else if (request.compare("GetLayInfo") == 0)
		{
			return GetLayInfo(request_body, result);
		}
	}

	return GetMap(doc_root, url, request_body, result);
}

bool WMSHandler::ClearAllDatasets(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	ResourcePool::GetInstance()->ClearDatasets();
	return true;
}

/*bool WMSHandler::GetRenderBytes(const std::list<std::pair<DatasetPtr, StylePtr>>& datasets, const Envelop& env, int tile_width, int tile_height, std::shared_ptr<HandleResult> result)
{
	BufferPtr buffer = TileProcessor::GetCombinedData(datasets, env, tile_width, tile_height);

	//if(buffer != nullptr)
	//{
	//	//test code
	//	FILE* pFile = nullptr;
	//	std::string path = "d:/test/";

	//	char string1[32];
	//	_itoa(nx, string1, 10);
	//	path += string1;
	//	path += "_";

	//	char string2[32];
	//	_itoa(ny, string2, 10);
	//	path += string2;
	//	path += "_";

	//	char string3[32];
	//	_itoa(nz, string3, 10);
	//	path += string3;

	//	path += ".jpg";

	//	fopen_s(&pFile, path.c_str(), "wb+");
	//	fwrite(buffer->data(), 1, buffer->size(), pFile);
	//	fclose(pFile);
	//	pFile = nullptr;
	//}

	if (buffer != nullptr)
	{
		result->set_buffer(buffer);

		http::buffer_body::value_type body;
		body.data = result->buffer()->data();
		body.size = result->buffer()->size();
		body.more = false;

		auto msg = std::make_shared<http::response<http::buffer_body>>(
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, result->version()));

		msg->set(http::field::server, BOOST_BEAST_VERSION_STRING);
		msg->set(http::field::content_type, "image/jpeg");

		msg->set(http::field::access_control_allow_origin, "*");
		msg->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
		msg->set(http::field::access_control_allow_credentials, "true");

		msg->content_length(result->buffer()->size());
		msg->keep_alive(result->keep_alive());

		result->set_buffer_body(msg);
	}

	return true;
}
*/

bool WMSHandler::GetDataStyleString(const Url& url, const std::string& request_body, std::string& data_style_json)
{
	//先判断url里有没有“key”
	std::string md5;
	if (url.QueryValue("key", md5))
	{
		if (!StorageManager::GetDataStyle(md5, data_style_json))
			return false;
	}
	else if (!request_body.empty())
	{
		data_style_json = request_body;
	}
	else if (!url.QueryValue("info", data_style_json))
	{
		return false;
	}

	return true;
}

bool WMSHandler::GetDatasets(int epsg_code, const std::string& data_style_json, std::list<std::pair<DatasetPtr, StylePtr>>& datasets)
{
	std::list<std::pair<std::string, std::string>> data_info;
	QueryDataInfo(data_style_json, data_info);

	for (auto info : data_info)
	{
		const std::string& path = info.first;
		std::string style_string = info.second;

		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(path));
		if (tiffDataset == nullptr)
			continue;

		StylePtr style_clone = StyleManager::GetStyle(style_string, tiffDataset);
		style_clone->set_code(epsg_code);
		datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
	}

	return true;
}

// bool WMSHandler::GetDatasets(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::list<std::pair<DatasetPtr, StylePtr>>& datasets)
// {
// 	int epsg_code = QuerySRS(url);
// 	if (epsg_code == -1)
// 	{
// 		//默认 web_mercator
// 		epsg_code = 3857;
// 	}
// 
// 	std::string data_style_json;
// 	if (!GetDataStyleString(url, request_body, data_style_json))
// 		return false;
// 
// 	std::list<std::pair<std::string, std::string>> data_info;
// 	QueryDataInfo(data_style_json, data_info);
// 
// 	for (auto info : data_info)
// 	{
// 		const std::string& path = info.first;
// 		std::string style_string = info.second;
// 
// 		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(path));
// 		if (tiffDataset == nullptr)
// 			continue;
// 
// 		StylePtr style_clone = StyleManager::GetStyle(style_string, tiffDataset);
// 		style_clone->set_code(epsg_code);
// 		datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
// 	}
// 
// 	return true;
// }

bool WMSHandler::GetHandleResult(bool use_cache, Envelop env, int tile_width, int tile_height, int epsg_code, const std::string& data_style, const std::string& amazon_md5, std::shared_ptr<HandleResult> result)
{
	BufferPtr buffer = nullptr;
	AmazonS3 amason_s3;
	std::string md5;
	if (use_cache && AmazonS3::GetUseS3())
	{
		buffer = amason_s3.GetS3Object(amazon_md5);
	}

	if (buffer == nullptr)
	{
		std::list<std::pair<DatasetPtr, StylePtr>> datasets;
		GetDatasets(epsg_code, data_style, datasets);

		buffer = TileProcessor::GetCombinedData(datasets, env, tile_width, tile_height);
		if (use_cache && AmazonS3::GetUseS3() && buffer != nullptr)
		{
			amason_s3.PutS3Object(md5, buffer);
		}
	}

	if (buffer != nullptr)
	{
		result->set_buffer(buffer);

		http::buffer_body::value_type body;
		body.data = result->buffer()->data();
		body.size = result->buffer()->size();
		body.more = false;

		auto msg = std::make_shared<http::response<http::buffer_body>>(
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, result->version()));

		msg->set(http::field::server, BOOST_BEAST_VERSION_STRING);
		msg->set(http::field::content_type, "image/jpeg");

		msg->set(http::field::access_control_allow_origin, "*");
		msg->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
		msg->set(http::field::access_control_allow_credentials, "true");

		msg->content_length(result->buffer()->size());
		msg->keep_alive(result->keep_alive());

		result->set_buffer_body(msg);
	}

	return true;
}

// 暂时不维护。如需要，参考WMTSHandler::GetTile进行修改
bool WMSHandler::GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	int epsg_code = QuerySRS(url);
	if (epsg_code == -1)
	{
		//默认 web_mercator
		epsg_code = 3857;
	}

	Envelop env;
	std::string env_string;
	if (url.QueryValue("bbox", env_string))
	{
		std::vector<std::string> tokens;
		Split(env_string, tokens, ",");
		if (tokens.size() != 4)
		{
			return false;
		}

		env.PutCoords(atof(tokens[0].c_str()), atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
	}
	else
	{
		return false;
	}

	int tile_width = QueryTileWidth(url);
	int tile_height = QueryTileHeight(url);

	std::string data_style;
	if (!GetDataStyleString(url, request_body, data_style))
		return false;

	bool use_cache = QueryIsUseCache(url);

	std::string md5;
	if (use_cache && AmazonS3::GetUseS3())
	{
		std::string amazon_data_style = data_style + env_string + std::to_string(tile_width) + std::to_string(tile_height) + std::to_string(epsg_code);
		GetMd5(md5, amazon_data_style.c_str(), amazon_data_style.size());
	}

	return GetHandleResult(use_cache, env, tile_width, tile_height, epsg_code, data_style, md5, result);
}

bool WMSHandler::UpdateDataStyle(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string res_string_body;
	http::status status_code = http::status::ok;

	std::string md5;
	if (StorageManager::AddOrUpdateDataStyle(request_body, md5))
	{
		res_string_body = md5;
		status_code = http::status::ok;
	}
	else
	{
		res_string_body = "UpdateDataStyle failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetLayInfo(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::vector<std::string> layers;
	GetLayers(request_body, layers);

	if (layers.empty())
		return false;

	std::vector<std::pair<Envelop, int>> envs;
	for (auto& path : layers)
	{
		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->GetDataset(path));
		if(tiffDataset != nullptr)
			envs.emplace_back(std::make_pair(tiffDataset->GetExtent(), tiffDataset->GetEPSG()));
		else
		{
			Envelop env;
			env.PutCoords(0, 0, 0, 0);
			envs.emplace_back(std::make_pair(env, -1));
		}
	}

	std::string json;
	GetGeojson(envs, json);

	auto string_body = CreateStringResponse(http::status::ok, result->version(), result->keep_alive(), json);
	result->set_string_body(string_body);
	
	return true;
}

bool WMSHandler::AddImages(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = request_body;
	if (!ImageGroupManager::AddImages(request_body))
	{
		res_string_body = "add images failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetImages(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body;
	if (!ImageGroupManager::GetImages(request_body, res_string_body))
	{
		res_string_body = "get images failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::ClearImages(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "OK";
	if (!ImageGroupManager::ClearImages(request_body))
	{
		res_string_body = "get images failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}