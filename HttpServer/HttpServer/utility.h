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

int GetDataTypeBytes(PIEDataType pieDataType);

struct Envelop
{
	double dLeft;
	double dTop;
	double dRight;
	double dBottom;
};

void GetEnvFromTileIndex(int nx, int ny, int nz, Envelop& env);

bool GetTileIndex(const std::string& path, int& nx, int& ny, int& nz);

bool LoadData(const std::string& path);

void ParseURL(const std::string& path, Envelop& env, std::string& filePath);