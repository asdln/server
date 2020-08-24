#ifndef HTTPSERVER_MAX_MIN_STRETCH_H_
#define HTTPSERVER_MAX_MIN_STRETCH_H_

#include "stretch.h"
class MaxMinStretch :
    public Stretch
{
public:

    void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType) override;

    double min_value();

    double max_value();

    void set_min_value(double min_value);

    void set_max_value(double max_value);

protected:

    double min_value_;
    double max_value_;
};

#endif //HTTPSERVER_MAX_MIN_STRETCH_H_