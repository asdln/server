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

bool TiffDataset::Read(int nx, int ny, int width, int height,
	void* pData, int bufferWidth, int bufferHeight, PIEDataType pieDataType,
	int nBandCount, int* pBandMap, long long pixSpace, long long lineSapce, long long bandSpace,
	void* psExtraArg)
{
	CPLErr error = poDataset_->RasterIO(GF_Read, nx, ny, width, height, pData, bufferWidth, bufferHeight, (GDALDataType)pieDataType, nBandCount, pBandMap, pixSpace, lineSapce, bandSpace, (GDALRasterIOExtraArg*)psExtraArg);
	return error == CE_None;

	return true;
}