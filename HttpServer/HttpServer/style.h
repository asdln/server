#ifndef HTTPSERVER_STYLE_H_
#define HTTPSERVER_STYLE_H_

#include <string>

class Style
{
public:

	Style() = default;

	virtual ~Style() = default;

protected:

	std::string kind;
};

#endif //HTTPSERVER_STYLE_H_