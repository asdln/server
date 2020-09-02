#include "min_max_stretch.h"


void MinMaxStretch::DoStretch(void* pData, unsigned char* pMaskBuffer, int nSize, int nBandCount, DataType dataType, double* no_data_value, int* have_no_data)
{
	auto pSrcData = nullptr;
	double* dstBuffer = nullptr;

	switch (dataType)
	{
	case DT_Byte:
	{
		DoStretchImpl((unsigned char*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_UInt16:
	{
		DoStretchImpl((unsigned short*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_Int16:
	{
		DoStretchImpl((short*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_UInt32:
	{
		DoStretchImpl((unsigned int*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_Int32:
	{
		DoStretchImpl((int*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_Float32:
	{
		DoStretchImpl((float*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_Float64:
	{
		DoStretchImpl((double*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_CInt16:
	{
		DoStretchImpl2((short*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_CInt32:
	{
		DoStretchImpl2((int*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat32:
	{
		DoStretchImpl2((float*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	case DT_CFloat64:
	{
		DoStretchImpl2((double*)pData, pMaskBuffer, nSize, nBandCount, no_data_value, have_no_data);
		break;
	}
	default:
		break;
	}
}
