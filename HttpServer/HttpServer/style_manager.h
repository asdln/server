#ifndef HTTPSERVER_STYLE_MANAGER_H_
#define HTTPSERVER_STYLE_MANAGER_H_

#include <string>
#include <memory>
#include "style.h"


class StyleManager
{
public:

	static bool UpdateStyle(const std::string& jsonStyle);

	static std::shared_ptr<Style> GetStyle(const std::string& kind = "");
};

#endif //HTTPSERVER_STYLE_MANAGER_H_