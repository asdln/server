#include "tiff_dataset.h"

#include "gdal_priv.h"

TiffDataset::~TiffDataset()
{
	Close();
}

void TiffDataset::Open(const std::string& path)
{
	GDALAllRegister();
	poDataset_ = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);

	poSpatialReference_ = (OGRSpatialReference*)OSRNewSpatialReference(poDataset_->GetProjectionRef());

	poDataset_->GetGeoTransform(dGeoTransform_);
	enumPIEDataType_ = (PIEDataType)poDataset_->GetRasterBand(1)->GetRasterDataType();

	int nWid = GetRasterXSize();
	int nHei = GetRasterYSize();

	double dx[4], dy[4];

	Pixel2World(0.0, 0.0, dx[0], dy[0]);
	Pixel2World(nWid, 0.0, dx[1], dy[1]);
	Pixel2World(nWid, nHei, dx[2], dy[2]);
	Pixel2World(0, nHei, dx[3], dy[3]);

	double dx1 = dx[0];
	double dx2 = dx[0];
	double dy1 = dy[0];
	double dy2 = dy[0];

	for (int i = 0; i < 4; i++)
	{
		if (dx1 > dx[i])
			dx1 = dx[i];

		if (dx2 < dx[i])
			dx2 = dx[i];

		if (dy1 > dy[i])
			dy1 = dy[i];

		if (dy2 < dy[i])
			dy2 = dy[i];
	}

	envelope_.PutCoords(dx1, dy1, dx2, dy2);
}

void TiffDataset::Close()
{
	if(poDataset_)
		GDALClose(poDataset_);

	poDataset_ = nullptr;

	if (poSpatialReference_)
	{
		OSRDestroySpatialReference(poSpatialReference_);
		poSpatialReference_ = nullptr;
	}
}

PIEDataType TiffDataset::GetDataType()
{
	return enumPIEDataType_;
}

OGRSpatialReference* TiffDataset::GetSpatialReference()
{
	return poSpatialReference_;
}

int TiffDataset::GetRasterXSize()
{
	if (poDataset_)
		return poDataset_->GetRasterXSize();

	return -1;
}

int TiffDataset::GetRasterYSize()
{
	if (poDataset_)
		return poDataset_->GetRasterYSize();

	return -1;
}

const Envelop& TiffDataset::GetExtent()
{
	return envelope_;
}

bool TiffDataset::World2Pixel(double dProjX, double dProjY, double& dCol, double& dRow)
{
	try
	{
		double dTemp = dGeoTransform_[1] * dGeoTransform_[5] - dGeoTransform_[2] * dGeoTransform_[4];

		dCol = (dGeoTransform_[5] * (dProjX - dGeoTransform_[0]) -
			dGeoTransform_[2] * (dProjY - dGeoTransform_[3])) / dTemp;
		dRow = (dGeoTransform_[1] * (dProjY - dGeoTransform_[3]) -
			dGeoTransform_[4] * (dProjX - dGeoTransform_[0])) / dTemp;

		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool TiffDataset::Pixel2World(double dCol, double dRow, double& dProjX, double& dProjY)
{
	dProjX = dGeoTransform_[0] + dCol * dGeoTransform_[1] + dRow * dGeoTransform_[2];
	dProjY = dGeoTransform_[3] + dCol * dGeoTransform_[4] + dRow * dGeoTransform_[5];

	return true;
}

bool TiffDataset::Read(int nx, int ny, int width, int height,
	void* pData, int bufferWidth, int bufferHeight, PIEDataType pieDataType,
	int nBandCount, int* pBandMap, long long pixSpace, long long lineSapce, long long bandSpace,
	void* psExtraArg)
{
	if (pixSpace == 0)
		pixSpace = GetDataTypeBytes(pieDataType) * nBandCount;

	if (lineSapce == 0)
		lineSapce = pixSpace * bufferWidth;

	if (bandSpace == 0)
		bandSpace = GetDataTypeBytes(pieDataType);

	CPLErr error = poDataset_->RasterIO(GF_Read, nx, ny, width, height, pData, bufferWidth, bufferHeight, (GDALDataType)pieDataType, nBandCount, pBandMap, pixSpace, lineSapce, bandSpace, (GDALRasterIOExtraArg*)psExtraArg);
	return error == CE_None;

	return true;
}