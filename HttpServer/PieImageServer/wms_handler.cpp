#include "wms_handler.h"
#include "style_manager.h"
#include "s3_dataset.h"
#include "resource_pool.h"
#include "storage_manager.h"
#include "image_group_manager.h"
#include "amazon_s3.h"
#include "coordinate_transformation.h"
#include "file_cache.h"
#include "etcd_storage.h"
#include "style_map.h"
#include "gdal_priv.h"

std::string g_s3_pyramid_dir;

unsigned char WMSHandler::cache_tag_ = 0;

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
		else if (request.compare("UpdateStyle") == 0)
		{
			return UpdateStyle(url, request_body, result);
		}
		else if (request.compare("GetStyle") == 0)
		{
			return GetStyle(url, result);
		}
		else if (request.compare("ClearAllDatasets") == 0)
		{
			return ClearAllDatasets(request_body, result);
		}
		else if (request.compare("AddImages") == 0)
		{
			return AddImages(request_body, result);
		}
		else if (request.compare("RemoveImages") == 0)
		{
			return RemoveImages(request_body, result);
		}
		else if (request.compare("GetImages") == 0)
		{
			return GetImages(request_body, result);
		}
		else if (request.compare("SetImages") == 0)
		{
			return SetImages(request_body, result);
		}
		else if (request.compare("GetGroups") == 0)
		{
			return GetGroups(result);
		}
		else if (request.compare("RemoveGroups") == 0)
		{
			return RemoveGroups(request_body, result);
		}
		else if (request.compare("AddGroups") == 0)
		{
			return AddGroups(request_body, result);
		}
		else if (request.compare("GetGroupEnvelope") == 0)
		{
			return GetGroupEnvelope(request_body, result);
		}
		else if (request.compare("ClearImages") == 0)
		{
			return ClearImages(request_body, result);
		}
		else if (request.compare("GetLayInfo") == 0)
		{
			return GetLayInfo(request_body, result);
		}
		else if (request.compare("GetEnvelope") == 0)
		{
			return GetEnvlope(request_body, result);
		}
		else if (request.compare("GetImageInfo") == 0)
		{
			return GetImageInfo(request_body, result);
		}
		else if (request.compare("GetGroupCacheState") == 0)
		{
			return GetGroupCacheState(request_body, result);
		}
		else if (request.compare("SetGroupCacheState") == 0)
		{
			return SetGroupCacheState(request_body, result);
		}
		else if (request.compare("ClearGroupCache") == 0)
		{
			return ClearGroupCache(request_body, result);
		}
	}

	return GetMap(doc_root, url, request_body, result);
}

bool WMSHandler::ClearAllDatasets(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	ResourcePool::GetInstance()->ClearDatasets();
	return true;
}

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

bool WMSHandler::GetStyleString(const Url& url, const std::string& request_body, std::string& style_json
	, std::list<std::string>& paths, Format& format, std::string& layerID)
{
	if (url.QueryValue("layer", layerID))
	{
		ImageGroupManager::GetImagesFromLocal(layerID, paths);

		std::string style_key;
		std::string data_style_json;
		if (url.QueryValue("style", style_key))
		{
			std::string format_str;
			url.QueryValue("format", format_str);
			format = String2Format(format_str);

			std::string style_md5;
			if (!style_key.empty() && !StyleMap::Find(style_key, style_md5))
				return false;

			if (!StorageManager::GetStyle(style_md5, style_json))
				return false;
		}
		else if (url.QueryValue("info", data_style_json))
		{
			if (!GetStyleStringFromInfoString(data_style_json, style_json))
				return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool WMSHandler::GetDatasets(int epsg_code, const std::string& data_style_json, bool one_to_one
	, const std::list<std::string>& data_paths, std::list<std::pair<DatasetPtr, StylePtr>>& datasets, Format& format)
{
	std::list<std::pair<std::string, std::string>> data_info;

	if (one_to_one)
	{
		QueryDataInfo(data_style_json, data_info, format);
	}
	else
	{
		std::list<std::string> style_strings;
		GetStylesFromStyleString(data_style_json, style_strings);
		std::list<std::string>::iterator itr = style_strings.begin();
		for (const auto& data_path : data_paths)
		{
			if (itr == style_strings.end())
			{
				data_info.emplace_back(std::make_pair(data_path, style_strings.back()));
			}
			else
			{
				data_info.emplace_back(std::make_pair(data_path, *itr));
				itr++;
			}
		}
	}

	for (auto info : data_info)
	{
		const std::string& path = info.first;
		std::string style_string = info.second;

		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->AcquireDataset(path));
		if (tiffDataset == nullptr)
			continue;

		std::shared_ptr<S3Dataset> s3Dataset = std::dynamic_pointer_cast<S3Dataset>(tiffDataset);
		if (s3Dataset != nullptr && !g_s3_pyramid_dir.empty() && s3Dataset->IsHavePyramid() == false)
		{
			s3Dataset->SetS3CacheKey(g_s3_pyramid_dir);
		}

		StylePtr style_clone = StyleManager::GetStyle(style_string, tiffDataset);
		style_clone->set_code(epsg_code);
		datasets.emplace_back(std::make_pair(tiffDataset, style_clone));
	}

	return true;
}

bool WMSHandler::GetHandleResult(bool use_cache, Envelop env, int tile_width, int tile_height, int epsg_code
	, const std::string& data_style, const std::string& amazon_md5, bool one_to_one, const std::list<std::string>& data_paths
	, Format format, bool statistic, Benchmark& bench_mark, std::shared_ptr<HandleResult> result)
{
	BufferPtr buffer = nullptr;
	std::mutex* p_mutex = nullptr;

	long long start_time = 0;
	GetCurrentTimeMilliseconds(start_time);

	if (use_cache && (S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache()))
	{
        //加锁，因为有可能出现多个请求写同一个图片的情况
		p_mutex = ResourcePool::GetInstance()->AcquireCacheMutex(amazon_md5);
		p_mutex->lock();

		if (cache_tag_ == 0)
		{
			std::cout << "using cache tile" << std::endl;
			cache_tag_ = 1;
		}

		if (S3Cache::GetUseS3Cache())
		{
			buffer = S3Cache::GetS3Object(amazon_md5);
		}
		else
		{
			buffer = FileCache::Read(amazon_md5);
		}
	}
	else if (cache_tag_ == 0)
	{
		std::cout << "not using cache tile" << std::endl;
		cache_tag_ = 1;
	}

	if (buffer == nullptr)
	{
		std::list<std::pair<DatasetPtr, StylePtr>> datasets;
		GetDatasets(epsg_code, data_style, one_to_one, data_paths, datasets, format);
		
		buffer = TileProcessor::GetCombinedData(datasets, env, tile_width, tile_height, format, bench_mark);
		if (use_cache && (S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache()) && buffer != nullptr)
		{
			if (S3Cache::GetUseS3Cache())
			{
				S3Cache::PutS3Object(amazon_md5, buffer);
			}
			else
			{
				FileCache::Write(amazon_md5, buffer);
			}
		}

		for (auto dataset : datasets)
		{
			ResourcePool::GetInstance()->ReleaseDataset(dataset.first);
		}
	}

	if (use_cache && (S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache()))
	{
		p_mutex->unlock();
		ResourcePool::GetInstance()->ReleaseCacheMutex(amazon_md5);
	}

	long long end_time = 0;
	GetCurrentTimeMilliseconds(end_time);

	bench_mark.TimeTag("GetHandleResult");

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
		msg->set(http::field::access_control_allow_methods, "POST, PUT, GET, OPTIONS, DELETE");
		msg->set(http::field::access_control_allow_credentials, "true");

		msg->content_length(result->buffer()->size());
		msg->keep_alive(result->keep_alive());

		if (statistic)
		{
			msg->set("Process_Time", std::to_string(end_time - start_time));

			auto last = bench_mark.time_statistics.begin();
			for (auto itr = bench_mark.time_statistics.begin(); itr != bench_mark.time_statistics.end(); itr++)
			{
				msg->set(itr->first, std::to_string(itr->second - last->second));
				last = itr;
			}

			int index = 0;
			for (const auto& read_statistic : bench_mark.read_statistics)
			{
				std::string str_index = std::to_string(index);
				msg->set("Read_Time" + str_index, std::to_string(read_statistic.read_time_milliseconds));
				msg->set("Read_X" + str_index, std::to_string(read_statistic.read_width));
				msg->set("Read_Y" + str_index, std::to_string(read_statistic.read_height));

				index++;
			}
		}

		result->set_buffer_body(msg);
	}

	return true;
}

// 暂时不维护。如需要，参考WMTSHandler::GetTile进行修改
bool WMSHandler::GetMap(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	QPSLocker qps_locker(g_qps);
	Benchmark benchmark(false);

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

	//style和path是否是一对一。如果url里有layer参数，则是多对一，即一个layer里的多个数据集对应同一个style
	bool one_to_one = true;

	std::list<std::string> data_paths;
	Format format = Format::WEBP;

	std::string json_str;
	std::string group;
	if (GetStyleString(url, request_body, json_str, data_paths, format, group))
	{
		one_to_one = false;
	}
	else if (!GetDataStyleString(url, request_body, json_str))
	{
		return false;
	}

	std::string md5;
	bool use_cache = false;
// 	bool use_cache = QueryIsUseCache(url);
// 
// 	if (use_cache && (AmazonS3::GetUseS3() || FileCache::GetUseFileCache()))
// 	{
// 		std::string amazon_data_style = json_str + env_string + std::to_string(tile_width) + std::to_string(tile_height) + std::to_string(epsg_code);
// 		GetMd5(md5, amazon_data_style.c_str(), amazon_data_style.size());
// 	}

	return GetHandleResult(use_cache, env, tile_width, tile_height, epsg_code
		, json_str, md5, one_to_one, data_paths, format, false, benchmark, result);
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

bool WMSHandler::GetStyle(const Url& url, std::shared_ptr<HandleResult> result)
{
	std::string style_id;
	if (!url.QueryValue("style", style_id))
		return false;

	std::string res_string_body;
	http::status status_code = http::status::ok;

	std::string style_md5;
	std::string data_style_json;
	if (StyleMap::Find(style_id, style_md5) && StorageManager::GetStyle(style_md5, data_style_json))
	{
		res_string_body = data_style_json;
	}
	else
	{
		res_string_body = "GetStyle failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::UpdateStyle(const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string style_id;
	if (!url.QueryValue("style", style_id))
		return false;

	std::string res_string_body;
	http::status status_code = http::status::ok;

	std::string md5;
	if (StorageManager::AddOrUpdateStyle(request_body, md5))
	{
		res_string_body = "ok";
		status_code = http::status::ok;

		StyleMap::Update(style_id, md5);
	}
	else
	{
		res_string_body = "UpdateStyle failed";
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
		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->AcquireDataset(path));
		if(tiffDataset != nullptr)
			envs.emplace_back(std::make_pair(tiffDataset->GetExtent(), tiffDataset->GetEPSG()));
		else
		{
			Envelop env;
			env.PutCoords(0, 0, 0, 0);
			envs.emplace_back(std::make_pair(env, -1));
		}

		ResourcePool::GetInstance()->ReleaseDataset(tiffDataset);
	}

	std::string json;
	GetGeojson(envs, json);

	auto string_body = CreateStringResponse(http::status::ok, result->version(), result->keep_alive(), json);
	result->set_string_body(string_body);
	
	return true;
}

bool WMSHandler::GetImageInfo(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::vector<std::string> layers;
	GetLayers(request_body, layers);

	if (layers.empty())
		return false;

	neb::CJsonObject oJson;

	std::vector<std::pair<Envelop, int>> envs;
	for (auto& path : layers)
	{
		neb::CJsonObject oJson_img;
		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->AcquireDataset(path));
		if (tiffDataset != nullptr)
		{
			int band_count = tiffDataset->GetBandCount();
			oJson_img.Add("band_count", band_count);

			const Envelop& env = tiffDataset->GetExtent();

			neb::CJsonObject oJson_env;
			oJson_env.Add("left", env.GetXMin());
			oJson_env.Add("right", env.GetXMax());
			oJson_env.Add("top", env.GetYMax());
			oJson_env.Add("bottom", env.GetYMin());

			oJson_env.Add("epsg", tiffDataset->GetEPSG());
			oJson_img.Add("envelope", oJson_env);
			oJson_img.Add("pyramid", tiffDataset->IsHavePyramid(), true);

			oJson_img.Add("width", tiffDataset->GetRasterXSize());
			oJson_img.Add("height", tiffDataset->GetRasterYSize());

			int block_x, block_y;
			tiffDataset->GetBlockSize(block_x, block_y);

			oJson_img.Add("block_size_x", block_x);
			oJson_img.Add("block_size_y", block_y);

			GDALColorTable* poColorTable = tiffDataset->GetColorTable(1);
			bool palette = poColorTable != nullptr;
			oJson_img.Add("palette", palette, true);

			if (palette)
			{
				neb::CJsonObject oJson_array_lut;

				for (int i = 0; i < 256; i ++)
				{
					neb::CJsonObject oJson_array_entry;
					const GDALColorEntry* poColorEntry = poColorTable->GetColorEntry(i);
					if (poColorEntry != nullptr)
					{
						oJson_array_entry.Add(i);
						oJson_array_entry.Add(poColorEntry->c1);
						oJson_array_entry.Add(poColorEntry->c2);
						oJson_array_entry.Add(poColorEntry->c3);
						oJson_array_entry.Add(poColorEntry->c4);
						oJson_array_lut.Add(oJson_array_entry);
					}
				}
				
				oJson_img.Add("lut", oJson_array_lut);
			}

			DataType data_type = tiffDataset->GetDataType();
			oJson_img.Add("type", GetDataTypeString(data_type));

			neb::CJsonObject oJson_bands;
			for (int i = 0; i < band_count; i++)
			{
				neb::CJsonObject ojson_band;
				ojson_band.Add("name", "B" + std::to_string(i + 1));

				double min, max, mean, std_dev;
				HistogramPtr histogram = ResourcePool::GetInstance()->GetHistogram(tiffDataset.get(), i + 1, false, 0.0);
				histogram->QueryStats(min, max, mean, std_dev);
				ojson_band.Add("min", min);
				ojson_band.Add("max", max);

				oJson_bands.Add(ojson_band);
			}

			oJson_img.Add("bands", oJson_bands);
			oJson.Add(oJson_img);
		}
		else
		{
			oJson.AddNull();
		}

		ResourcePool::GetInstance()->ReleaseDataset(tiffDataset);
	}

	std::string json = oJson.ToString();

	auto string_body = CreateStringResponse(http::status::ok, result->version(), result->keep_alive(), json);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetEnvlope(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::vector<std::string> layers;
	GetLayers(request_body, layers);

	if (layers.empty())
		return false;

	std::vector<std::pair<Envelop, int>> envs;
	for (auto& path : layers)
	{
		std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->AcquireDataset(path));
		if (tiffDataset != nullptr)
		{
			int epsg = tiffDataset->GetEPSG();
			if (epsg != 4326)
			{
				Envelop dst;
				const Envelop env = tiffDataset->GetExtent();

				OGRSpatialReference* ptrDSSRef = ResourcePool::GetInstance()->GetSpatialReference(4326);
				CoordinateTransformation transformation2(tiffDataset->GetSpatialReference(), ptrDSSRef);
				bool res = transformation2.Transform(env, dst);
				if (res)
				{
					envs.emplace_back(std::make_pair(dst, 4326));
				}
				else
				{
					Envelop env;
					env.PutCoords(0, 0, 0, 0);
					envs.emplace_back(std::make_pair(env, -1));
				}
			}
			else
			{
				envs.emplace_back(std::make_pair(tiffDataset->GetExtent(), tiffDataset->GetEPSG()));
			}
		}
		else
		{
			Envelop env;
			env.PutCoords(0, 0, 0, 0);
			envs.emplace_back(std::make_pair(env, -1));
		}

		ResourcePool::GetInstance()->ReleaseDataset(tiffDataset);
	}

	std::string json;
	GetGeojson(envs, json);

	auto string_body = CreateStringResponse(http::status::ok, result->version(), result->keep_alive(), json);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetGroupEnvelope(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::vector<std::string> groups;
	GetLayers(request_body, groups);

	if (groups.empty())
		return false;

	std::vector<std::pair<Envelop, int>> envs;

	for (auto& group : groups)
	{
		std::list<std::string> image_paths;
		ImageGroupManager::GetImagesFromLocal(group, image_paths);

		Envelop group_env;
		int epsg_group = -9999;
		for (auto& image_path : image_paths)
		{
			std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(ResourcePool::GetInstance()->AcquireDataset(image_path));
			if (tiffDataset != nullptr)
			{
				int epsg = tiffDataset->GetEPSG();
				Envelop env = tiffDataset->GetExtent();

				if (epsg != 4326)
				{
					OGRSpatialReference* ptrDSSRef = ResourcePool::GetInstance()->GetSpatialReference(4326);
					OGRSpatialReference* ptrSrcRef = tiffDataset->GetSpatialReference();
					if (ptrSrcRef != nullptr)
					{
						CoordinateTransformation transformation2(ptrSrcRef, ptrDSSRef);
						bool res = transformation2.Transform(env, env);
					}
				}

				if (group_env.IsEmpty())
				{
					epsg_group = epsg;
					group_env = env;
				}
				else if(epsg == epsg_group)
				{
					group_env.Union(env);
				}
			}

			ResourcePool::GetInstance()->ReleaseDataset(tiffDataset);
		}

		envs.emplace_back(std::make_pair(group_env, epsg_group));
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
	std::string res_string_body = "ok";
	if (!ImageGroupManager::AddImages(request_body))
	{
		res_string_body = "add images failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::RemoveImages(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::RemoveImages(request_body))
	{
		res_string_body = "remove images failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetGroups(std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::GetAllGroup(res_string_body))
	{
		res_string_body = "null";
		status_code = http::status::internal_server_error;
	}

	if (res_string_body.empty())
	{
		res_string_body = "null";
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::RemoveGroups(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::RemoveGroups(request_body))
	{
		res_string_body = "null";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::AddGroups(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::AddGroups(request_body))
	{
		res_string_body = "null";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::SetImages(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::SetImages(request_body))
	{
		res_string_body = "set images failed";
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
		res_string_body = "null";
		status_code = http::status::internal_server_error;
	}

	if (res_string_body.empty())
	{
		res_string_body = "null";
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
		res_string_body = "ClearImages failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::GetGroupCacheState(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "OK";
	if (!ImageGroupManager::GetGroupCacheState(request_body, res_string_body))
	{
		res_string_body = "null";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::SetGroupCacheState(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	cache_tag_ = 0;
	S3Cache::ResetTag();

	http::status status_code = http::status::ok;
	std::string res_string_body = "ok";
	if (!ImageGroupManager::SetGroupCacheState(request_body))
	{
		res_string_body = "set cache state failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}

bool WMSHandler::ClearGroupCache(const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	http::status status_code = http::status::ok;
	std::string res_string_body = "OK";
	if (!ImageGroupManager::ClearGroupCache(request_body))
	{
		res_string_body = "ClearGroupCache failed";
		status_code = http::status::internal_server_error;
	}

	auto string_body = CreateStringResponse(status_code, result->version(), result->keep_alive(), res_string_body);
	result->set_string_body(string_body);

	return true;
}