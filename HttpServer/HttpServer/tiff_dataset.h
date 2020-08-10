#pragma once
#include "dataset_interface.h"

class GDALDataset;

class TiffDataset :
    public DatasetInterface
{
public:

    virtual void Open(const std::string& path) override;

    virtual void Close() override;

    virtual bool Read(const Envelop& env, void* pData, int width, int height, PixelDataType pixelType) override;

protected:

    bool Projection2ImageRowCol(double* adfGeoTransform, double dProjX, double dProjY, double& dCol, double& dRow);

private:

    GDALDataset* poDataset_;
};

