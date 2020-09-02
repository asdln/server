#ifndef HTTPSERVER_STRETCH_INTERFACE_H_
#define HTTPSERVER_STRETCH_INTERFACE_H_

#include "utility.h"

enum class StretchType
{
	MINIMUM_MAXIMUM
};

class Stretch
{
public: 
	virtual void DoStretch(void* pData, unsigned char* pMaskBuffer, int nSize, int nBandCount, DataType dataType, double* no_data_value, int* have_no_data) {};

	StretchType kind_;
};

#endif HTTPSERVER_STRETCH_INTERFACE_H_