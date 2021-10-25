#pragma once

#include <string>
#include <vector>
#include <list>
#include "type_def.h"

//int GetDataTypeBytes(DataType DataType);

std::string GetDataTypeString(DataType data_type);

void CreateUID(std::string& uid);

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env, int epsg);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

bool LoadData(const std::string& path);

//void ParseURL(const std::string& path, Envelop& env, std::string& filePath);

void Split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ");

bool GetMd5(std::string& str_md5, const char* const buffer, size_t buffer_size);

void CreateUID(std::string& uid);

void QueryDataInfo(const std::string& request_body
	, std::list<std::tuple<std::string, std::string, std::string>>& data_info
	, Format& format, int& width, int& height);

void GetLayers(const std::string& request_body, std::vector<std::string>& paths);

void GetGroups(const std::string& request_body, std::vector<std::string>& groups);

void GetGeojson(const std::vector<std::tuple<Envelop, int, std::string>>& envs, std::string& json);

bool GetStyleStringFromInfoString(const std::string& info_string, std::string& styles_string);

void GetStylesFromStyleString(const std::string& styles_string, std::list<std::string>& style_strings);