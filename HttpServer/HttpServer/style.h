#ifndef HTTPSERVER_STYLE_H_
#define HTTPSERVER_STYLE_H_

#include <string>

class Style
{
public:

	Style() = default;

	virtual ~Style() = default;

	const std::string& kind() { return kind_; }

protected:

	std::string kind_;
};

#endif //HTTPSERVER_STYLE_H_