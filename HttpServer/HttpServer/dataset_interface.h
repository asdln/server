#pragma once

#include "utility.h"

class DatasetInterface
{
public:

	virtual void Open(const std::string& path) = 0;

	virtual void Close() = 0;

	virtual bool Read(const Envelop& env, void* pData, int width, int height, PixelDataType pixelType) = 0;
};

