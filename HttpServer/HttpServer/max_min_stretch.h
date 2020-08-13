#ifndef HTTPSERVER_MAX_MIN_STRETCH_H_
#define HTTPSERVER_MAX_MIN_STRETCH_H_

#include "stretch_interface.h"
class MaxMinStretch :
    public StretchInterface
{
public:

    void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType) override;
};

#endif //HTTPSERVER_MAX_MIN_STRETCH_H_