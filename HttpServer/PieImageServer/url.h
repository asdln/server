#ifndef HTTPSERVER_URL_H_
#define HTTPSERVER_URL_H_

#include <string>
#include <unordered_map>

class Url
{
public:

	Url(const std::string& url);

	const std::string& protocol();

	const std::string& domain();

	const std::string& port();

	const std::string& path();

	const std::string& query();

	bool QueryValue(const std::string& key, std::string& value) const;

protected:

	std::string UrlDecode(std::string const& str);

	std::string UrlEncode(std::string const& str);

	std::string HexEncode(char const c);

	char HexDecode(std::string const& s);

private:

	std::string protocol_ = "";
	std::string domain_ = "";
	std::string port_ = "";
	std::string path_ = "";
	std::string query_ = "";
	std::unordered_multimap<std::string, std::string> queryKV_;
};

#endif //HTTPSERVER_URL_H_