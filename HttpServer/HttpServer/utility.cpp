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
				if (keyValue[0].compare("x") == 0 || keyValue[0].compare("X") == 0)
				{
					nx = atoi(keyValue[1].c_str());
				}
				else if (keyValue[0].compare("y") == 0 || keyValue[0].compare("Y") == 0)
				{
					ny = atoi(keyValue[1].c_str());
				}
				else if (keyValue[0].compare("z") == 0 || keyValue[0].compare("Z") == 0)
				{
					nz = atoi(keyValue[1].c_str());
				}
			}
		}
	}

	return true;
}
