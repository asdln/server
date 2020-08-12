#pragma once

#include "utility.h"
class OGRSpatialReference;

class DatasetInterface
{
public:

	virtual void Open(const std::string& path) = 0;

	virtual void Close() = 0;

	virtual bool Read(int, int, int, int,
		void*, int, int, PIEDataType,
		int, int*, long long = 0, long long = 0, long long = 0,
		void* psExtraArg = nullptr) = 0;

	virtual int GetRasterXSize() = 0;

	virtual int GetRasterYSize() = 0;

	virtual const Envelop& GetExtent() = 0;

	virtual bool World2Pixel(double dProjX, double dProjY, double& dCol, double& dRow) = 0;

	virtual bool Pixel2World(double dCol, double dRow, double& dProjX, double& dProjY) = 0;

	virtual PIEDataType GetDataType() = 0;

	virtual OGRSpatialReference* GetSpatialReference() = 0;

	virtual ~DatasetInterface() {};
};

