#ifndef HTTPSERVER_COORDINATE_TRANSFORMATION_H_
#define HTTPSERVER_COORDINATE_TRANSFORMATION_H_

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

	bool need_tranverse_before_ = false;
	bool need_tranverse_after_ = false;
};

#endif //HTTPSERVER_COORDINATE_TRANSFORMATION_H_