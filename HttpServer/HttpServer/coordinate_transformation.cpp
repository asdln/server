#include "coordinate_transformation.h"

#include "gdal_priv.h"
#include "math.h"
#include <algorithm>

CoordinateTransformation::CoordinateTransformation(OGRSpatialReference* pSrc, OGRSpatialReference* pDst)
{
	pOGRCoordinateTransformation_ = OGRCreateCoordinateTransformation(pSrc, pDst);
}

bool CoordinateTransformation::Transform(int count, double* pX, double* pY, double* pZ)
{
	int nRes = pOGRCoordinateTransformation_->Transform(count, pX, pY, pZ);
	return nRes;
}

bool CoordinateTransformation::Transform(const Envelop& srcEnv, Envelop& dstEnv)
{
	double dx1 = srcEnv.GetXMin();
	double dy1 = srcEnv.GetYMax();

	double dx2 = srcEnv.GetXMin();
	double dy2 = srcEnv.GetYMin();

	double dx3 = srcEnv.GetXMax();
	double dy3 = srcEnv.GetYMax();

	double dx4 = srcEnv.GetXMax();
	double dy4 = srcEnv.GetYMin();

	bool bRes1 = pOGRCoordinateTransformation_->Transform(1, &dx1, &dy1);
	bool bRes2 = pOGRCoordinateTransformation_->Transform(1, &dx2, &dy2);
	bool bRes3 = pOGRCoordinateTransformation_->Transform(1, &dx3, &dy3);
	bool bRes4 = pOGRCoordinateTransformation_->Transform(1, &dx4, &dy4);

	double dxMin = std::min(std::min(dx1, dx2), std::min(dx3, dx4));
	double dyMin = std::min(std::min(dy1, dy2), std::min(dy3, dy4));

	double dxMax = std::max(std::max(dx1, dx2), std::max(dx3, dx4));
	double dyMax = std::max(std::max(dy1, dy2), std::max(dy3, dy4));

	dstEnv.PutCoords(dxMin, dyMin, dxMax, dyMax);

	return bRes1 && bRes2 && bRes3 && bRes4;
}

CoordinateTransformation::~CoordinateTransformation()
{
	OGRCoordinateTransformation::DestroyCT(pOGRCoordinateTransformation_);
	pOGRCoordinateTransformation_ = nullptr;
}