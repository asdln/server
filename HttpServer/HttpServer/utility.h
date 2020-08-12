#pragma once

#include <string>

class utility
{
};

enum PixelDataType
{
	PIXEL_TYPE_RGB = 0,
	PIXEL_TYPE_RGBA
};

typedef enum {
	/*! Unknown or unspecified type */          PIE_Unknown = 0,
	/*! Eight bit unsigned integer */           PIE_Byte = 1,
	/*! Sixteen bit unsigned integer */         PIE_UInt16 = 2,
	/*! Sixteen bit signed integer */           PIE_Int16 = 3,
	/*! Thirty two bit unsigned integer */      PIE_UInt32 = 4,
	/*! Thirty two bit signed integer */        PIE_Int32 = 5,
	/* TODO?(#6879): GDT_UInt64 */
	/* TODO?(#6879): GDT_Int64 */
	/*! Thirty two bit floating point */        PIE_Float32 = 6,
	/*! Sixty four bit floating point */        PIE_Float64 = 7,
	/*! Complex Int16 */                        PIE_CInt16 = 8,
	/*! Complex Int32 */                        PIE_CInt32 = 9,
	/* TODO?(#6879): GDT_CInt64 */
	/*! Complex Float32 */                      PIE_CFloat32 = 10,
	/*! Complex Float64 */                      PIE_CFloat64 = 11,
	GDT_TypeCount = 12          /* maximum type # + 1 */
} PIEDataType;

enum RasterResamplingType
{
	NEAREST_NEIGHBOR = 0,
	BILINEAR_INTERPOLATION = 1
};

int GetDataTypeBytes(PIEDataType pieDataType);

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
		maxX = origin.GetXMax() < dMaxX_ ? dMaxX_ : origin.GetXMax();
		maxY = origin.GetYMax() < dMaxY_ ? dMaxY_ : origin.GetYMax();

		result.PutCoords(minX, minY, maxX, maxY);

		return true;
	}

private:

	double dMinX_ = 0.0;
	double dMinY_ = 0.0;

	double dMaxX_ = 0.0;
	double dMaxY_ = 0.0;
};

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

bool LoadData(const std::string& path);

void ParseURL(const std::string& path, Envelop& env, std::string& filePath);