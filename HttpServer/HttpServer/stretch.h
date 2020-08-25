#ifndef HTTPSERVER_STRETCH_INTERFACE_H_
#define HTTPSERVER_STRETCH_INTERFACE_H_

#include "utility.h"
#include "enums.h"

class Stretch
{
public: 
	virtual void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType) {};

	StretchType kind_;
};

#endif HTTPSERVER_STRETCH_INTERFACE_H_