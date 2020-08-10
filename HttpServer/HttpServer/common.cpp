#include "common.h"


int GetPixelDataTypeSize(PixelDataType type)
{
	int nSize = 1;
	switch (type)
	{
	case PIXEL_TYPE_RGB:
		nSize = 3;
		break;
	case PIXEL_TYPE_RGBA:
		nSize = 4;
		break;
	default:
		break;
	}

	return nSize;
}