#ifndef HTTPSERVER_TIFF_DATASET_H_
#define HTTPSERVER_TIFF_DATASET_H_

#include "dataset.h"

class GDALDataset;

class TiffDataset :
    public Dataset
{
public:

    virtual void Open(const std::string& path) override;

    virtual void Close() override;

    virtual const std::string& file_path() override;

	virtual bool Read(int nx, int ny, int width, int height,
		void* pData, int bufferWidth, int bufferHeight, DataType DataType,
		int nBandCount, int* pBandMap, long long pixSpace = 0, long long lineSapce = 0, long long bandSpace = 0,
		void* psExtraArg = nullptr) override;

    virtual int GetRasterXSize() override;

    virtual int GetRasterYSize() override;

    virtual const Envelop& GetExtent() override;

    virtual bool World2Pixel(double dProjX, double dProjY, double& dCol, double& dRow) override;

    virtual bool Pixel2World(double dCol, double dRow, double& dProjX, double& dProjY) override;

    virtual DataType GetDataType() override;

    virtual OGRSpatialReference* GetSpatialReference() override;

    virtual double GetNoDataValue(int band, int* pbSuccess = nullptr) override;

    virtual HistogramPtr GetHistogram(int band) override;

    virtual int GetBandCount() override;

    bool IsUseRPC() { return m_bUsePRC; }

    ~TiffDataset();

protected:

    void CalcExtent();


private:

    GDALDataset* poDataset_ = nullptr;

    OGRSpatialReference* poSpatialReference_ = nullptr;

    double dGeoTransform_[6];

    DataType enumDataType_;

    Envelop envelope_;

	void* rpc_transform_arg;

	bool m_bUsePRC;

    std::string file_path_;
};

#endif //HTTPSERVER_TIFF_DATASET_H_