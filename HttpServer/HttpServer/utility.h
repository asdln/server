#pragma once

#include <string>

class utility
{
};

enum PixelDataType
{
	PIXEL_TYPE_RGB = 0,
	PIXEL_TYPE_RGBA
};

int GetPixelDataTypeSize(PixelDataType type);

struct Envelop
{
	double dLeft;
	double dTop;
	double dRight;
	double dBottom;
};

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

bool LoadData(const std::string& path);

void GetInfo(const std::string& path, Envelop& env, std::string& filePath);