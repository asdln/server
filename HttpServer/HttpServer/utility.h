#pragma once

#include <string>

class utility
{
};

struct Envelop
{
	double dLeft;
	double dTop;
	double dRight;
	double dBottom;
};

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

void GetInfo(const std::string& path, Envelop& env, std::string& filePath);