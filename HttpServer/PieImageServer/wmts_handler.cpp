#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "resource_pool.h"
#include "storage_manager.h"
#include "amazon_s3.h"
#include "file_cache.h"
#include "image_group_manager.h"
#include "benchmark.h"

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

bool WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url
	, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, request_body, result);
		}
		else
		{
			return WMSHandler::Handle(doc_root, url, request_body, result);
		}

		return false;
	}

	return GetTile(doc_root, url, request_body, result);
}

bool WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url
	, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	bool statistic = QueryStatistic(url);

	QPSLocker qps_locker(g_qps);
	Benchmark benchmark(statistic);

	benchmark.TimeTag("GetTile_Begin");

	int epsg_code = QuerySRS(url);
	if (epsg_code == -1)
	{
		//默认 web_mercator
		epsg_code = 3857;
	}

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env, epsg_code);

	//style和path是否是一对一。如果url里有layer参数，则是多对一，即一个layer里的多个数据集对应同一个style，或者多个style
	bool one_to_one = true;

	std::list<std::string> data_paths;
	Format format = Format::WEBP;

	std::string json_str;
	std::string group;
	bool use_cache = true;
	if (GetStyleString(url, request_body, json_str, data_paths, format, group))
	{
		one_to_one = false;
	}
	else if (!GetDataStyleString(url, request_body, json_str))
	{
		return false;
	}

	if (one_to_one == true)
	{
		use_cache = QueryIsUseCache(url);
	}
	else
	{
		use_cache = ImageGroupManager::GetGroupCacheState(group);
	}

	std::string md5;
	if (use_cache && (S3Cache::GetUseS3Cache() || FileCache::GetUseFileCache()))
	{
		std::string amazon_data_style = json_str + std::to_string(nx)
			+ std::to_string(ny) + std::to_string(nz) + std::to_string(epsg_code);
		if (one_to_one == false)
		{
			//路径也要加进来，作为md5的参数
			for (auto& path : data_paths)
			{
				amazon_data_style += path;
			}
		}

		GetMd5(md5, amazon_data_style.c_str(), amazon_data_style.size());
	}

	//如果是有图层的，则按图层名为目录存放缓存
	if (one_to_one == false && use_cache == true)
	{
		md5 = group + '/' + md5;
	}

	return GetHandleResult(use_cache, env, 256, 256, epsg_code, json_str
		, md5, one_to_one, data_paths, format, statistic, benchmark, result);
}