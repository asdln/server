#include "tiff_dataset.h"

#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "gdal_alg.h"
#include <fstream>

TiffDataset::~TiffDataset()
{
	delete mem_pool_;
	Close();
}

bool TiffDataset::Open(const std::string& path)
{
	file_path_ = path;

	std::string s3pre = "s3://";
	if (strncmp(path.c_str(), s3pre.c_str(), s3pre.length()) == 0)
	{
		std::string new_path = path.substr(s3pre.size(), path.size() - s3pre.size());
		new_path = "/vsis3/" + new_path;

		poDataset_ = (GDALDataset*)GDALOpen(new_path.c_str(), GA_ReadOnly);
	}
	else
	{
		poDataset_ = (GDALDataset*)GDALOpen(path.c_str(), GA_ReadOnly);
	}

	if (poDataset_ == nullptr)
		return false;

	const char* strDes = poDataset_->GetDriver()->GetDescription();
	bool bSearchRPC = false;

#ifdef __GNUC__

    if (strcasecmp(strDes, "GTiff") == 0 || strcasecmp(strDes, "ENVI") == 0
        || strcasecmp(strDes, "HFA") == 0 || strcasecmp(strDes, "JP2ECW") == 0)
    {
        bSearchRPC = true;
    }

#else

    if (_stricmp(strDes, "GTiff") == 0 || _stricmp(strDes, "ENVI") == 0
        || _stricmp(strDes, "HFA") == 0 || _stricmp(strDes, "JP2ECW") == 0)
    {
        bSearchRPC = true;
    }

#endif



	GDALRPCInfo sRPCInfo;
	char** papszMD = NULL;
	char** papszOptions = NULL;

	enumDataType_ = (DataType)poDataset_->GetRasterBand(1)->GetRasterDataType();

	if (bSearchRPC && (papszMD = GDALGetMetadata(poDataset_, "RPC")) != nullptr
		&& GDALExtractRPCInfo(papszMD, &sRPCInfo))
	{
		rpc_transform_arg =
			GDALCreateRPCTransformer(&sRPCInfo, FALSE, 0, papszOptions);

		m_bUsePRC = true;

		const char* strWKT = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9108\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
		poSpatialReference_ = (OGRSpatialReference*)OSRNewSpatialReference(strWKT);
	}
	else
	{
		if (poDataset_->GetSpatialRef() != nullptr)
		{
			poSpatialReference_ = poDataset_->GetSpatialRef()->Clone();
		}
		
		poDataset_->GetGeoTransform(dGeoTransform_);

		if (std::string(poDataset_->GetProjectionRef()).empty())
		{
			dGeoTransform_[0] = 0;
			dGeoTransform_[1] = 1;
			dGeoTransform_[2] = 0;
			dGeoTransform_[3] = 0;
			dGeoTransform_[4] = 0;
			dGeoTransform_[5] = -1;
		}
	}

	CalcExtent();

	return true;
}

bool TiffDataset::IsHavePyramid()
{
	GDALRasterBand* poRasterBand = poDataset_->GetRasterBand(1);
	if (poRasterBand != nullptr && poRasterBand->GetOverviewCount() != 0)
	{
		return true;
	}

	return false;
}

void TiffDataset::CalcExtent()
{
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

DataType TiffDataset::GetDataType()
{
	return enumDataType_;
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

GDALColorTable* TiffDataset::GetColorTable(int band)
{
	GDALRasterBand* poRasterBand = poDataset_->GetRasterBand(band);
	if (poRasterBand == nullptr)
		return nullptr;

	return poRasterBand->GetColorTable();
}

void TiffDataset::GetBlockSize(int& x, int& y)
{
	GDALRasterBand* poBand1 = poDataset_->GetRasterBand(1);
	poBand1->GetBlockSize(&x, &y);
}

const Envelop& TiffDataset::GetExtent()
{
	return envelope_;
}

int TiffDataset::GetEPSG()
{
	if (m_bUsePRC)
		return 4326;

	if (std::string(poDataset_->GetProjectionRef()).empty())
		return -1;

	poSpatialReference_->AutoIdentifyEPSG();

	if (poSpatialReference_->IsProjected())
	{
		const char* pszAuthName, * pszAuthCode;

		pszAuthName = poSpatialReference_->GetAuthorityName("PROJCS");
		pszAuthCode = poSpatialReference_->GetAuthorityCode("PROJCS");

		if (pszAuthName == NULL || pszAuthCode == NULL)
		{
			return -1;
		}

		if (EQUAL(pszAuthName, "EPSG"))
		{
			return atoi(pszAuthCode);
		}
	}
	else if(poSpatialReference_->IsGeographic())
	{
		return poSpatialReference_->GetEPSGGeogCS();
	}

	return -1;
}

std::string TiffDataset::GetWKT()
{
	if (std::string(poDataset_->GetProjectionRef()).empty())
		return nullptr;

	char* wkt = nullptr;
	if (poSpatialReference_)
		poSpatialReference_->exportToWkt(&wkt);

	return std::string(wkt);
}

bool TiffDataset::World2Pixel(double dProjX, double dProjY, double& dCol, double& dRow)
{
	if (m_bUsePRC)
	{
		double padfZ = 0.0;
		int panSuccess = 0;
		GDALRPCTransform(rpc_transform_arg, true,
			1, &dProjX, &dProjY, &padfZ,
			&panSuccess);

		dCol = dProjX;
		dRow = dProjY;
	}
	else
	{
		try
		{
			double dTemp = dGeoTransform_[1] * dGeoTransform_[5] - dGeoTransform_[2] * dGeoTransform_[4];

			dCol = (dGeoTransform_[5] * (dProjX - dGeoTransform_[0]) -
				dGeoTransform_[2] * (dProjY - dGeoTransform_[3])) / dTemp;
			dRow = (dGeoTransform_[1] * (dProjY - dGeoTransform_[3]) -
				dGeoTransform_[4] * (dProjX - dGeoTransform_[0])) / dTemp;
		}
		catch (...)
		{
			return false;
		}
	}

	return true;
}

typedef enum
{
	/*! Nearest neighbour (select on one input pixel) */
	DRA_NearestNeighbour = 0,
	/*! Bilinear (2x2 kernel) */
	DRA_Bilinear = 1,
	/*! Cubic Convolution Approximation (4x4 kernel) */
	DRA_Cubic = 2
} DEMResampleAlg;

typedef struct
{
	GDALTransformerInfo sTI;
	GDALRPCInfo sRPC;
	double      adfPLToLatLongGeoTransform[6];
	double      dfRefZ;
	int         bReversed;
	double      dfPixErrThreshold;
	double      dfHeightOffset;
	double      dfHeightScale;
	char* pszDEMPath;
	DEMResampleAlg eResampleAlg;
	int         bHasDEMMissingValue;
	double      dfDEMMissingValue;
	int         bApplyDEMVDatumShift;
	int         bHasTriedOpeningDS;
	GDALDataset* poDS;
	OGRCoordinateTransformation* poCT;
	int         nMaxIterations;
	double      adfDEMGeoTransform[6];
	double      adfDEMReverseGeoTransform[6];

#ifdef USE_SSE2_OPTIM
	double      adfDoubles[20 * 4 + 1];
	double* padfCoeffs; // LINE_NUM_COEFF, LINE_DEN_COEFF, SAMP_NUM_COEFF and then SAMP_DEN_COEFF
#endif

	bool        bRPCInverseVerbose;
	char* pszRPCInverseLog;
} GDALRPCTransformInfo;

bool TiffDataset::Pixel2World(double dCol, double dRow, double& dProjX, double& dProjY)
{
	if (m_bUsePRC)
	{
		double padfZ = 0.0;
		int panSuccess = 0;
		GDALRPCTransform(rpc_transform_arg, false,
			1, &dCol, &dRow, &padfZ,
			&panSuccess);

		if (panSuccess != 1)
		{
			GDALRPCTransformInfo* psTransform = (GDALRPCTransformInfo*)rpc_transform_arg;
			dProjX = psTransform->adfPLToLatLongGeoTransform[0]
				+ psTransform->adfPLToLatLongGeoTransform[1] * dCol
				+ psTransform->adfPLToLatLongGeoTransform[2] * dRow;

			dProjY = psTransform->adfPLToLatLongGeoTransform[3]
				+ psTransform->adfPLToLatLongGeoTransform[4] * dCol
				+ psTransform->adfPLToLatLongGeoTransform[5] * dRow;

			return true;
		}

		dProjX = dCol;
		dProjY = dRow;
	}
	else
	{
		dProjX = dGeoTransform_[0] + dCol * dGeoTransform_[1] + dRow * dGeoTransform_[2];
		dProjY = dGeoTransform_[3] + dCol * dGeoTransform_[4] + dRow * dGeoTransform_[5];
	}

	return true;
}

bool TiffDataset::Read(int nx, int ny, int width, int height,
	void* pData, int bufferWidth, int bufferHeight, DataType dataType,
	int nBandCount, int* pBandMap, long long pixSpace, long long lineSapce, long long bandSpace,
	void* psExtraArg)
{
	if (pixSpace == 0)
		pixSpace = GetDataTypeBytes(dataType) * nBandCount;

	if (lineSapce == 0)
		lineSapce = pixSpace * bufferWidth;

	if (bandSpace == 0)
		bandSpace = GetDataTypeBytes(dataType);

	CPLErr error = poDataset_->RasterIO(GF_Read, nx, ny, width, height, pData, bufferWidth, bufferHeight
		, (GDALDataType)dataType, nBandCount, pBandMap, pixSpace, lineSapce, bandSpace, (GDALRasterIOExtraArg*)psExtraArg);

	return error == CE_None;
}

double TiffDataset::GetNoDataValue(int band, int* pbSuccess)
{
	return poDataset_->GetRasterBand(band)->GetNoDataValue(pbSuccess);
}

HistogramPtr TiffDataset::GetHistogram(int band)
{
	return nullptr;
}

int TiffDataset::GetBandCount()
{
	return poDataset_->GetRasterCount();
}

const std::string& TiffDataset::file_path()
{
	return file_path_;
}

bool TiffDataset::ReadHistogramFile(std::string& file_content)
{
	std::string histogram_file = file_path_ + ".histogram.json";

	std::ifstream inFile2(histogram_file, std::ios::binary | std::ios::in);
	if (inFile2.is_open())
	{
		inFile2.seekg(0, std::ios_base::end);
		int FileSize2 = inFile2.tellg();

		inFile2.seekg(0, std::ios::beg);

		std::vector<unsigned char> buffer;
		//char* buffer2 = new char[FileSize2];
		buffer.resize(FileSize2);
		inFile2.read((char*)buffer.data(), FileSize2);
		inFile2.close();

		std::string string_json(buffer.begin(), buffer.end());
		file_content = string_json;

		return true;
	}

	return false;
}

bool TiffDataset::SaveHistogramFile(const std::string& file_content)
{
	std::string histogram_file = file_path_ + ".histogram.json";

	std::ofstream outFile(histogram_file, std::ios::binary | std::ios::out);
	if (outFile.is_open())
	{
		outFile.write(file_content.data(), file_content.size());
		outFile.close();

		return true;
	}

	return false;
}