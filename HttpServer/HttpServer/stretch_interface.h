#pragma once

#include "utility.h"

class StretchInterface
{
public: 
	virtual void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], PIEDataType pieDataType) = 0;
};

