#include "coordinate_transformation.h"

#include "gdal_priv.h"
#include "math.h"
#include <algorithm>

CoordinateTransformation::CoordinateTransformation(OGRSpatialReference* pSrc, OGRSpatialReference* pDst)
{
	if (pSrc->IsGeographic() && pDst->IsGeographic())
	{
		need_tranverse_before_ = true;
	}
	else if (pSrc->IsGeographic())
	{
		need_tranverse_before_ = true;
		need_tranverse_after_ = true;
	}
	else if (pDst->IsGeographic())
	{
		need_tranverse_after_ = true;
	}
	else if (!pDst->IsGeographic() && !pSrc->IsGeographic()) //暂时如此，不知道是否通用
	{
		need_tranverse_after_ = true;
	}

	pOGRCoordinateTransformation_ = OGRCreateCoordinateTransformation(pSrc, pDst);
}

bool CoordinateTransformation::Transform(int count, double* pX, double* pY, double* pZ)
{
	if (pOGRCoordinateTransformation_ == nullptr)
		return false;

	int nRes = 0;
	if (need_tranverse_before_)
	{
		nRes = pOGRCoordinateTransformation_->Transform(count, pY, pX, pZ);
	}
	else
	{
		nRes = pOGRCoordinateTransformation_->Transform(count, pX, pY, pZ);
	}
	

//#ifndef GDAL_V2

	if (need_tranverse_after_)
	{
		double temp;
		for (int i = 0; i < count; i++)
		{
			temp = pX[i];
			pX[i] = pY[i];
			pY[i] = temp;
		}
	}

//#endif

	return nRes;
}

bool CoordinateTransformation::Transform(const Envelop& srcEnv, Envelop& dstEnv)
{
	if (pOGRCoordinateTransformation_ == nullptr)
		return false;

	double dx1 = srcEnv.GetXMin();
	double dy1 = srcEnv.GetYMax();

	double dx2 = srcEnv.GetXMin();
	double dy2 = srcEnv.GetYMin();

	double dx3 = srcEnv.GetXMax();
	double dy3 = srcEnv.GetYMax();

	double dx4 = srcEnv.GetXMax();
	double dy4 = srcEnv.GetYMin();

	bool bRes1 = false;
	bool bRes2 = false;
	bool bRes3 = false;
	bool bRes4 = false;

	if (need_tranverse_before_)
	{
		bRes1 = pOGRCoordinateTransformation_->Transform(1, &dy1, &dx1);
		bRes2 = pOGRCoordinateTransformation_->Transform(1, &dy2, &dx2);
		bRes3 = pOGRCoordinateTransformation_->Transform(1, &dy3, &dx3);
		bRes4 = pOGRCoordinateTransformation_->Transform(1, &dy4, &dx4);
	}
	else
	{
		bRes1 = pOGRCoordinateTransformation_->Transform(1, &dx1, &dy1);
		bRes2 = pOGRCoordinateTransformation_->Transform(1, &dx2, &dy2);
		bRes3 = pOGRCoordinateTransformation_->Transform(1, &dx3, &dy3);
		bRes4 = pOGRCoordinateTransformation_->Transform(1, &dx4, &dy4);
	}

//#ifndef GDAL_V2
	if (need_tranverse_after_)
	{
		double temp;
		temp = dx1;
		dx1 = dy1;
		dy1 = temp;

		temp = dx2;
		dx2 = dy2;
		dy2 = temp;

		temp = dx3;
		dx3 = dy3;
		dy3 = temp;

		temp = dx4;
		dx4 = dy4;
		dy4 = temp;
	}

//#endif

	double dxMin = std::min(std::min(dx1, dx2), std::min(dx3, dx4));
	double dyMin = std::min(std::min(dy1, dy2), std::min(dy3, dy4));

	double dxMax = std::max(std::max(dx1, dx2), std::max(dx3, dx4));
	double dyMax = std::max(std::max(dy1, dy2), std::max(dy3, dy4));

	dstEnv.PutCoords(dxMin, dyMin, dxMax, dyMax);

	return bRes1 && bRes2 && bRes3 && bRes4;
}

CoordinateTransformation::~CoordinateTransformation()
{
	if (pOGRCoordinateTransformation_ != nullptr)
	{
		OGRCoordinateTransformation::DestroyCT(pOGRCoordinateTransformation_);
		pOGRCoordinateTransformation_ = nullptr;
	}
}