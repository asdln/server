#include "utility.h"

#include "math.h"
#include <vector>

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env)
{
	size_t nTileCountX = pow(2.0, nz) + 0.5;
	size_t nTileCountY = nTileCountX;

	int nTileSize = 256;

	double dTotalLeft = -180.0;
	double dTotalTop = 180.0;

	double dResoltion = 360.0 / (nTileCountX * 256.0);

	env.dLeft = dTotalLeft + nx * (nTileSize * dResoltion);
	env.dTop = -180.0 + (nTileCountY - ny) * nTileSize * dResoltion;
	env.dRight = env.dLeft + nTileSize * dResoltion;
	env.dBottom = env.dTop - nTileSize * dResoltion;
}

void split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ")
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
	filePath = "d:/work/world.tif";
}

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz)
{
	std::vector<std::string> tokens;
	split(path, tokens, "?");

	if (tokens.size() < 2)
		return false;

	const std::string& token = tokens.back();

	std::vector<std::string> params;
	split(token, params, "&");

	for (auto& param : params)
	{
		size_t index = param.find('=');
		if (index != -1)
		{
			std::vector<std::string> keyValue;
			split(param, keyValue, "=");
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

int GetDataTypeBytes(PIEDataType pieDataType)
{
	int nBytes = 0;
	switch (pieDataType)
	{
	case PIE_Byte:
	{
		nBytes = 1;
		break;
	}
	case PIE_UInt16:
	{
		nBytes = 2;
		break;
	}
	case PIE_Int16:
	{
		nBytes = 2;
		break;
	}
	case PIE_UInt32:
	{
		nBytes = 4;
		break;
	}
	case PIE_Int32:
	{
		nBytes = 4;
		break;
	}
	case PIE_Float32:
	{
		nBytes = 4;
		break;
	}
	case PIE_Float64:
	{
		nBytes = 8;
		break;
	}
	case PIE_CInt16:
	{
		nBytes = 2;
		break;
	}
	case PIE_CInt32:
	{
		nBytes = 4;
		break;
	}
	case PIE_CFloat32:
	{
		nBytes = 4;
		break;
	}
	case PIE_CFloat64:
	{
		nBytes = 8;
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