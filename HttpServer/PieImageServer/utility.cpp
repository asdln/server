#include "utility.h"

#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "math.h"
#include <vector>

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env)
{
	size_t nTileCountX = pow(2.0, nz) + 0.5;
	size_t nTileCountY = nTileCountX;

	int nTileSize = 256;

	double dTotalLeft = -180.0;
	double dTotalBottom = -180.0;

	double dResoltion = 360.0 / (nTileCountX * 256.0);

	bool bWebMercator = 1;
	if (bWebMercator)
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

void ParseURL(const std::string& path, Envelop& env, std::string& filePath)
{
	int nx = 0;
	int ny = 0;
	int nz = 0;

	GetTileIndex(path, nx, ny, nz);
	GetEnvFromTileIndex(nx, ny, nz, env);
	//filePath = "d:/work/world.tif";
	//filePath = "D:\\work\\langfang_base - 2m\\langfang_mosaic.img";
	filePath = "D:\\work\\data\\GF1_PMS2_E113.9_N34.4_20181125_L5A_0003622463.tiff";
}

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

int GetDataTypeBytes(DataType DataType)
{
	int nBytes = 0;
	switch (DataType)
	{
	case DT_Byte:
	{
		nBytes = 1;
		break;
	}
	case DT_UInt16:
	{
		nBytes = 2;
		break;
	}
	case DT_Int16:
	{
		nBytes = 2;
		break;
	}
	case DT_UInt32:
	{
		nBytes = 4;
		break;
	}
	case DT_Int32:
	{
		nBytes = 4;
		break;
	}
	case DT_Float32:
	{
		nBytes = 4;
		break;
	}
	case DT_Float64:
	{
		nBytes = 8;
		break;
	}
	case DT_CInt16:
	{
		nBytes = 4;
		break;
	}
	case DT_CInt32:
	{
		nBytes = 8;
		break;
	}
	case DT_CFloat32:
	{
		nBytes = 8;
		break;
	}
	case DT_CFloat64:
	{
		nBytes = 16;
		break;
	}
	default:
	{
		nBytes = 1;
		break;
	}
	}

	return nBytes;
}

void CreateUID(std::string& uid)
{
	auto uidGen = boost::uuids::random_generator();
	boost::uuids::uuid uid1 = uidGen();
	std::stringstream sGuid;
	sGuid << uid1;
	uid = sGuid.str();
}