#include "wmts_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"
#include "utility.h"
#include "tiff_dataset.h"
#include "resource_pool.h"
#include "storage_manager.h"
#include "amazon_s3.h"
#include "file_cache.h"

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

bool WMTSHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
	std::string request;
	if (url.QueryValue("request", request))
	{
		if (request.compare("GetTile") == 0)
		{
			return GetTile(doc_root, url, request_body, result);
		}
		else if (request.compare("UpdateDataStyle") == 0)
		{
			return UpdateDataStyle(request_body, result);
		}
		else if (request.compare("UpdateStyle") == 0)
		{
			return UpdateStyle(request_body, result);
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

		return false;
	}

	return GetTile(doc_root, url, request_body, result);
}

bool WMTSHandler::GetTile(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, std::shared_ptr<HandleResult> result)
{
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

	//style和path是否是一对一。如果url里有layer参数，则是多对一，即一个layer里的多个数据集对应同一个style
	bool one_to_one = true;

	std::list<std::string> data_paths;
	std::string cachekey;
	Format format = Format::WEBP;

	std::string json_str;
	if (GetStyleString(url, request_body, json_str, data_paths, cachekey, format))
	{
		one_to_one = false;
	}
	else if (!GetDataStyleString(url, request_body, json_str))
	{
		return false;
	}

	bool use_cache = QueryIsUseCache(url);

	std::string md5;
	if (use_cache && (AmazonS3::GetUseS3() || FileCache::GetUseFileCache()))
	{
		std::string amazon_data_style = json_str + std::to_string(nx) + std::to_string(ny) + std::to_string(nz) + std::to_string(epsg_code);
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

	return GetHandleResult(use_cache, env, 256, 256, epsg_code, json_str, md5, one_to_one, data_paths, cachekey, format, result);
}