#pragma once
#include "dataset_interface.h"

class GDALDataset;

class TiffDataset :
    public DatasetInterface
{
public:

    virtual void Open(const std::string& path) override;

    virtual void Close() override;

	virtual bool Read(int nx, int ny, int width, int height,
		void* pData, int bufferWidth, int bufferHeight, PIEDataType pieDataType,
		int nBandCount, int* pBandMap, long long pixSpace, long long lineSapce, long long bandSpace,
		void* psExtraArg = nullptr) override;

    virtual int GetRasterXSize() override;

    virtual int GetRasterYSize() override;

    virtual bool World2Pixel(double dProjX, double dProjY, double& dCol, double& dRow) override;

    virtual PIEDataType GetDataType() override;

    virtual OGRSpatialReference* GetSpatialReference() override;

    ~TiffDataset();

protected:


private:

    GDALDataset* poDataset_ = nullptr;

    OGRSpatialReference* poSpatialReference_ = nullptr;

    double dGeoTransform_[6];

    PIEDataType enumPIEDataType_;
};

