#include "type_def.h"


int GetDataTypeBytes(DataType DataType)
{
	int nBytes = 0;
	switch (DataType)
	{
	case DT_Byte:
	{
		nBytes = 1;
		break;
	}
	case DT_UInt16:
	{
		nBytes = 2;
		break;
	}
	case DT_Int16:
	{
		nBytes = 2;
		break;
	}
	case DT_UInt32:
	{
		nBytes = 4;
		break;
	}
	case DT_Int32:
	{
		nBytes = 4;
		break;
	}
	case DT_Float32:
	{
		nBytes = 4;
		break;
	}
	case DT_Float64:
	{
		nBytes = 8;
		break;
	}
	case DT_CInt16:
	{
		nBytes = 4;
		break;
	}
	case DT_CInt32:
	{
		nBytes = 8;
		break;
	}
	case DT_CFloat32:
	{
		nBytes = 8;
		break;
	}
	case DT_CFloat64:
	{
		nBytes = 16;
		break;
	}
	default:
	{
		nBytes = 1;
		break;
	}
	}

	return nBytes;
}