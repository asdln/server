#ifndef HTTPSERVER_STYLE_H_
#define HTTPSERVER_STYLE_H_

#include <string>
#include <memory>
#include "min_max_stretch.h"

#include "enums.h"

struct Style
{
	std::string uid_ = "";

	size_t version_ = 0;

	StyleType kind_ = StyleType::TRUE_COLOR;
	int bandMap_[4] = {1, 2, 3, 4};
	int bandCount_ = 3;

	std::shared_ptr<Stretch> stretch_ = std::make_shared<MinMaxStretch>();
};

#endif //HTTPSERVER_STYLE_H_