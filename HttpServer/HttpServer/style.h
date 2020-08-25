#ifndef HTTPSERVER_STYLE_H_
#define HTTPSERVER_STYLE_H_

#include <string>
#include <memory>
#include "stretch.h"

#include "enums.h"

struct Style
{
	std::string uid_;

	size_t version_;

	StyleType kind_;
	int bandMap_[4];
	int bandCount_;

	std::shared_ptr<Stretch> stretch_;
};

#endif //HTTPSERVER_STYLE_H_