#ifndef HTTPSERVER_STRETCH_INTERFACE_H_
#define HTTPSERVER_STRETCH_INTERFACE_H_

#include "utility.h"

class StretchInterface
{
public: 
	virtual void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType) = 0;
};

#endif HTTPSERVER_STRETCH_INTERFACE_H_