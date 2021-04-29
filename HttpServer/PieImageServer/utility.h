#pragma once

#include <string>
#include <vector>
#include <list>

enum class StyleType
{
	NONE = 0,
	TRUE_COLOR,
	DEM,
	GRAY
};

enum class Format
{
	JPG = 0,
	PNG,
	WEBP
};


enum PixelFormat
{
	PIXEL_FORMAT_RGB = 0,
	PIXEL_FORMAT_RGBA
};

typedef enum {
	/*! Unknown or unspecified type */          DT_Unknown = 0,
	/*! Eight bit unsigned integer */           DT_Byte = 1,
	/*! Sixteen bit unsigned integer */         DT_UInt16 = 2,
	/*! Sixteen bit signed integer */           DT_Int16 = 3,
	/*! Thirty two bit unsigned integer */      DT_UInt32 = 4,
	/*! Thirty two bit signed integer */        DT_Int32 = 5,
	/* TODO?(#6879): GDT_UInt64 */
	/* TODO?(#6879): GDT_Int64 */
	/*! Thirty two bit floating point */        DT_Float32 = 6,
	/*! Sixty four bit floating point */        DT_Float64 = 7,
	/*! Complex Int16 */                        DT_CInt16 = 8,
	/*! Complex Int32 */                        DT_CInt32 = 9,
	/* TODO?(#6879): GDT_CInt64 */
	/*! Complex Float32 */                      DT_CFloat32 = 10,
	/*! Complex Float64 */                      DT_CFloat64 = 11,
	DT_TypeCount = 12          /* maximum type # + 1 */
} DataType;

enum RasterResamplingType
{
	NEAREST_NEIGHBOR = 0,
	BILINEAR_INTERPOLATION = 1
};

int GetDataTypeBytes(DataType DataType);

void CreateUID(std::string& uid);

class Envelop
{
public:

	void QueryCoords(double& dMinX, double& dMinY, double& dMaxX, double& dMaxY) const
	{
		dMinX = dMinX_;
		dMinY = dMinY_;

		dMaxX = dMaxX_;
		dMaxY = dMaxY_;
	}

	void PutCoords(double dMinX, double dMinY, double dMaxX, double dMaxY)
	{
		dMinX_ = dMinX;
		dMinY_ = dMinY;

		dMaxX_ = dMaxX;
		dMaxY_ = dMaxY;
	}

	double GetYMax()const { return dMaxY_; }

	double GetXMax()const { return dMaxX_; }

	double GetYMin()const { return dMinY_; }

	double GetXMin()const { return dMinX_; }

	double GetWidth() const { return dMaxX_ - dMinX_; }

	double GetHeight() const { return dMaxY_ - dMinY_; }

	void Normalize()
	{
		if (dMinX_ > dMaxX_)
			std::swap(dMinX_, dMaxX_);

		if (dMinY_ > dMaxY_)
			std::swap(dMinY_, dMaxY_);
	}

	bool Intersects(const Envelop& another) const
	{
		if (another.GetXMax() < GetXMin()
			|| another.GetXMin() > GetXMax()
			|| another.GetYMax() < GetYMin()
			|| another.GetYMin() > GetYMax())
			return false;

		return true;
	}

	bool Intersection(const Envelop& origin, Envelop& result) const
	{
		if (!Intersects(origin))
			return false;

		double minX, minY, maxX, maxY;

		minX = origin.GetXMin() < dMinX_ ? dMinX_ : origin.GetXMin();

		minY = origin.GetYMin() < dMinY_ ? dMinY_ : origin.GetYMin();
		maxX = origin.GetXMax() > dMaxX_ ? dMaxX_ : origin.GetXMax();
		maxY = origin.GetYMax() > dMaxY_ ? dMaxY_ : origin.GetYMax();

		result.PutCoords(minX, minY, maxX, maxY);

		return true;
	}

private:

	double dMinX_ = 0.0;
	double dMinY_ = 0.0;

	double dMaxX_ = 0.0;
	double dMaxY_ = 0.0;
};

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env, int epsg);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

bool LoadData(const std::string& path);

//void ParseURL(const std::string& path, Envelop& env, std::string& filePath);

void Split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ");

bool GetMd5(std::string& str_md5, const char* const buffer, size_t buffer_size);

void CreateUID(std::string& uid);

void QueryDataInfo(const std::string& request_body, std::list<std::pair<std::string, std::string>>& data_info, Format& format);

void GetLayers(const std::string& request_body, std::vector<std::string>& paths);

void GetGeojson(const std::vector<std::pair<Envelop, int>>& envs, std::string& json);