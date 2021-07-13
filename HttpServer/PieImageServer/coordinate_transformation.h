#pragma once

#include "utility.h"

class OGRSpatialReference;
class OGRCoordinateTransformation;

class CoordinateTransformation
{
public:

	CoordinateTransformation(OGRSpatialReference* pSrc, OGRSpatialReference* pDst);

	bool Transform(int count, double* pX, double* pY, double* pZ = nullptr);

	bool Transform(const Envelop& srcEnv, Envelop& dstEnv);

	~CoordinateTransformation();

private:

	OGRCoordinateTransformation* pOGRCoordinateTransformation_ = nullptr;
};