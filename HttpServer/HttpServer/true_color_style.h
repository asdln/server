#ifndef HTTPSERVER_TRUE_COLOR_STYLE_H_
#define HTTPSERVER_TRUE_COLOR_STYLE_H_

#include <memory>
#include "style.h"
#include "stretch.h"
#include "max_min_stretch.h"

class TrueColorStyle :
    public Style
{
public:

    TrueColorStyle() = default;

    std::shared_ptr<Stretch> stretch();

    void set_stretch(std::shared_ptr<Stretch> stretch);

    int band_count();

    void set_band_count(int band_count);

protected:

    int band_count_ = 3;

    int band_map_[3] = { 1, 2, 3 };

    std::shared_ptr<Stretch> stretch_ = std::make_shared<MaxMinStretch>();
};

#endif //HTTPSERVER_TRUE_COLOR_STYLE_H_