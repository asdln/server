#include "utility.h"

#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/lexical_cast.hpp>

#include "math.h"
#include <vector>
#include "CJsonObject.hpp"

//************************************
// Method:        ReplaceStringInStd
// Describe:        使用指定子串替换字符串中的字符串，如果要替换的字符串为空，则移除原序列中的指定子串
// FullName:      ReplaceStringInStd
// Access:          public 
// Returns:        std::string
// Returns Describe:    返回替换操作后的字符串副本
// Parameter:    std::string strOrigin      ；原字符串
// Parameter:    std::string strToReplace   ；需要替换的字符换
// Parameter:    std::string strNewChar     ；替换后的字符换，当该字符串为空时，该函数的功能变成
//************************************
std::string ReplaceStringInStd(const std::string& strOrigin, const std::string& strToReplace, const std::string& strNewChar)
{
	std::string strFinal = strOrigin;
	if (strFinal.empty())
	{
		return strFinal;
	}

	if (strNewChar.empty())
	{
		size_t pos = std::string::npos;

		// Search for the substring in string in a loop untill nothing is found
		while ((pos = strFinal.find(strToReplace)) != std::string::npos)
		{
			// If found then erase it from string
			strFinal.erase(pos, strToReplace.length());
		}
	}
	else
	{
		for (std::string::size_type pos(0); pos != std::string::npos; pos += strNewChar.length())
		{
			pos = strFinal.find(strToReplace, pos);
			if (pos != std::string::npos)
				strFinal.replace(pos, strToReplace.length(), strNewChar);
			else
				break;
		}
	}
	return   strFinal;
}

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env, int epsg)
{
	size_t nTileCountX = pow(2.0, nz) + 0.5;
	size_t nTileCountY = nTileCountX;

	int nTileSize = 256;

	double dTotalLeft = -180.0;
	double dTotalBottom = -180.0;

	double dResoltion = 360.0 / (nTileCountX * 256.0);

	if (epsg == 3857)
	{
		dTotalLeft = -20037508.3427892;
		dTotalBottom = -20037508.3427892;

		dResoltion = 20037508.3427892 * 2.0 / (nTileCountX * 256.0);
	}

	double dx1, dy1, dx2, dy2;

	dx1 = dTotalLeft + nx * (nTileSize * dResoltion);
	dy2 = dTotalBottom + (nTileCountY - ny) * nTileSize * dResoltion;
	dx2 = dx1 + nTileSize * dResoltion;
	dy1 = dy2 - nTileSize * dResoltion;

	env.PutCoords(dx1, dy1, dx2, dy2);
}

void Split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters)
{
	std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
	std::string::size_type pos = s.find_first_of(delimiters, lastPos);
	while (std::string::npos != pos || std::string::npos != lastPos) {
		tokens.push_back(s.substr(lastPos, pos - lastPos));
		lastPos = s.find_first_not_of(delimiters, pos);
		pos = s.find_first_of(delimiters, lastPos);
	}
}

bool LoadData(const std::string& path)
{
	return true;
}

//void ParseURL(const std::string& path, Envelop& env, std::string& filePath)
//{
//	int nx = 0;
//	int ny = 0;
//	int nz = 0;
//
//	GetTileIndex(path, nx, ny, nz);
//	GetEnvFromTileIndex(nx, ny, nz, env);
//	//filePath = "d:/work/world.tif";
//	//filePath = "D:\\work\\langfang_base - 2m\\langfang_mosaic.img";
//	filePath = "D:\\work\\data\\GF1_PMS2_E113.9_N34.4_20181125_L5A_0003622463.tiff";
//}

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz)
{
	std::vector<std::string> tokens;
	Split(path, tokens, "?");

	if (tokens.size() < 2)
		return false;

	const std::string& token = tokens.back();

	std::vector<std::string> params;
	Split(token, params, "&");

	for (auto& param : params)
	{
		size_t index = param.find('=');
		if (index != -1)
		{
			std::vector<std::string> keyValue;
			Split(param, keyValue, "=");
			if (keyValue.size() == 2)
			{
				if (keyValue[0].compare("x") == 0 || keyValue[0].compare("X") == 0 || keyValue[0].compare("TILECOL") == 0)
				{
					nx = atoi(keyValue[1].c_str());
				}
				else if (keyValue[0].compare("y") == 0 || keyValue[0].compare("Y") == 0 || keyValue[0].compare("TILEROW") == 0)
				{
					ny = atoi(keyValue[1].c_str());
				}
				else if (keyValue[0].compare("z") == 0 || keyValue[0].compare("Z") == 0 || keyValue[0].compare("TILEMATRIX") == 0)
				{
					nz = atoi(keyValue[1].c_str());
				}
			}
		}
	}

	return true;
}

std::string GetDataTypeString(DataType data_type)
{
	std::string type_string;
	switch (data_type)
	{
	case DT_Byte:
	{
		type_string = "uint8";
		break;
	}
	case DT_UInt16:
	{
		type_string = "uint16";
		break;
	}
	case DT_Int16:
	{
		type_string = "int16";
		break;
	}
	case DT_UInt32:
	{
		type_string = "uint32";
		break;
	}
	case DT_Int32:
	{
		type_string = "int32";
		break;
	}
	case DT_Float32:
	{
		type_string = "float32";
		break;
	}
	case DT_Float64:
	{
		type_string = "float64";
		break;
	}
	case DT_CInt16:
	{
		type_string = "cint16";;
		break;
	}
	case DT_CInt32:
	{
		type_string = "cint32";
		break;
	}
	case DT_CFloat32:
	{
		type_string = "cfloat32";
		break;
	}
	case DT_CFloat64:
	{
		type_string = "cfloat64";
		break;
	}
	default:
	{
		type_string = "unknown";
		break;
	}
	}

	return type_string;
}

bool GetMd5(std::string& str_md5, const char* const buffer, size_t buffer_size)
{
	if (buffer == nullptr)
	{
		return false;
	}
	boost::uuids::detail::md5 boost_md5;
	boost_md5.process_bytes(buffer, buffer_size);
	boost::uuids::detail::md5::digest_type digest;
	boost_md5.get_digest(digest);
	const auto char_digest = reinterpret_cast<const char*>(&digest);
	str_md5.clear();
	boost::algorithm::hex(char_digest, char_digest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(str_md5));
	return true;
}

void CreateUID(std::string& uid)
{
	auto uidGen = boost::uuids::random_generator();
	boost::uuids::uuid uid1 = uidGen();
	std::stringstream sGuid;
	sGuid << uid1;
	uid = sGuid.str();
}

Format String2Format(const std::string& format_string);

void QueryDataInfo(const std::string& request_body
	, std::list<std::tuple<std::string, std::string, std::string>>& data_info
	, Format& format, int& width, int& height)
{
	neb::CJsonObject oJson_info;
	neb::CJsonObject oJson(request_body);
	std::string format_string;
	if (oJson.Get("info", oJson_info))
	{
		if (oJson_info.IsArray())
		{
			int array_size = oJson_info.GetArraySize();
			for (int i = 0; i < array_size; i++)
			{
				neb::CJsonObject oJson_dataset_info = oJson_info[i];
				std::string path = "";
				std::string s3cachekey = "";
				std::string style_string = "";
				neb::CJsonObject oJson_style;

				if (oJson_dataset_info.Get("path", path))
				{
					//style_string 可以为空
					if (oJson_dataset_info.Get("style", oJson_style))
					{
						style_string = oJson_style.ToString();
					}

					oJson_dataset_info.Get("s3cachekey", s3cachekey);
					data_info.emplace_back(std::make_tuple(path, style_string, s3cachekey));
				}
			}
		}
	}

	format = String2Format(oJson("format"));
	oJson.Get("width", width);
	oJson.Get("height", height);
}

void GetLayers(const std::string& request_body, std::vector<std::string>& paths)
{
	neb::CJsonObject oJson_info;
	neb::CJsonObject oJson(request_body);

	if (oJson.IsArray())
	{
		int array_size = oJson.GetArraySize();
		for (int i = 0; i < array_size; i++)
		{
			std::string path;
			oJson.Get(i, path);
			paths.emplace_back(path);
		}
	}
}

void GetGroups(const std::string& request_body, std::vector<std::string>& groups)
{
	GetLayers(request_body, groups);
}

void GetGeojson(const std::vector<std::tuple<Envelop, int, std::string>>& envs, std::string& json)
{
	neb::CJsonObject oJson;
	for (const auto& env : envs)
	{
		neb::CJsonObject oJson_env;
		oJson_env.Add("left", std::get<0>(env).GetXMin());
		oJson_env.Add("right", std::get<0>(env).GetXMax());
		oJson_env.Add("top", std::get<0>(env).GetYMax());
		oJson_env.Add("bottom", std::get<0>(env).GetYMin());

		oJson_env.Add("epsg", std::get<1>(env));
		oJson_env.Add("wkt", std::get<2>(env));
		oJson.Add(oJson_env);
	}

	json = oJson.ToString();
}

bool GetStyleStringFromInfoString(const std::string& info_string, std::string& styles_string)
{
	neb::CJsonObject oJson(info_string);

	neb::CJsonObject styles;
	oJson.Get("info", styles);

	if (styles.IsArray())
	{
		styles_string = styles.ToString();
		return true;
	}

	return false;
}

void GetStylesFromStyleString(const std::string& styles_string, std::list<std::string>& style_strings)
{
	neb::CJsonObject oJson(styles_string);
	if (oJson.IsArray())
	{
		int array_size = oJson.GetArraySize();
		for (int i = 0; i < array_size; i++)
		{
			neb::CJsonObject oJson_sub;
			oJson.Get(i, oJson_sub);

			//std::string style_string;
			neb::CJsonObject oJson_style;
			oJson_sub.Get("style", oJson_style);
			style_strings.emplace_back(oJson_style.ToString());
		}
	}
	else
	{
		neb::CJsonObject oJson_style;
		oJson.Get("style", oJson_style);
		style_strings.emplace_back(oJson_style.ToString());
	}
}