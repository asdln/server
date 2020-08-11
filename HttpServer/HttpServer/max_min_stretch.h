#pragma once
#include "stretch_interface.h"
class MaxMinStretch :
    public StretchInterface
{
public:

    void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], PIEDataType pieDataType) override;
};

