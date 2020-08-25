#ifndef HTTPSERVER_MAX_MIN_STRETCH_H_
#define HTTPSERVER_MAX_MIN_STRETCH_H_

#include "stretch.h"
class MaxMinStretch :
    public Stretch
{
public:

    MaxMinStretch() = default;

    MaxMinStretch(double dMin, double dMax) { min_value_ = dMin; max_value_ = dMax; }

    void DoStretch(void* pData, int nSize, int nBandCount, int bandMap[], DataType DataType) override;

    double min_value();

    double max_value();

    void set_min_value(double min_value);

    void set_max_value(double max_value);

public:

    double min_value_ = 0.0;
    double max_value_ = 0.0;
};

#endif //HTTPSERVER_MAX_MIN_STRETCH_H_