#include "url.h"

#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "utility.h"


Url::Url(const std::string& url)
{
	//boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
	boost::regex ex("(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
	boost::cmatch what;
	if (regex_match(url.c_str(), what, ex))
	{
		path_ = std::string(what[1].first, what[1].second);
		query_ = std::string(what[2].first, what[2].second);

		std::vector<std::string> params;
		Split(query_, params, "&");

		for (auto& param : params)
		{
			size_t index = param.find('=');
			if (index != -1)
			{
				std::vector<std::string> keyValue;
				Split(param, keyValue, "=");
				if (keyValue.size() == 2)
				{
					queryKV_.emplace(boost::to_lower_copy(UrlDecode(keyValue[0])), UrlDecode(keyValue[1]));
				}
			}
		}
	}
}


char Url::HexDecode(std::string const& s)
{
	unsigned int n;

	std::sscanf(s.data(), "%x", &n);

	return static_cast<char>(n);
}

std::string Url::UrlDecode(std::string const& str)
{
	std::string res;
	res.reserve(str.size());

	for (std::size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == '+')
		{
			res += " ";
		}
		else if (str[i] == '%' && i + 2 < str.size() &&
			std::isxdigit(static_cast<unsigned char>(str[i + 1])) &&
			std::isxdigit(static_cast<unsigned char>(str[i + 2])))
		{
			res += HexDecode(str.substr(i + 1, 2));
			i += 2;
		}
		else
		{
			res += str[i];
		}
	}

	return res;
}

std::string Url::UrlEncode(std::string const& str)
{
	std::string res;
	res.reserve(str.size());

	for (auto const& e : str)
	{
		if (e == ' ')
		{
			res += "+";
		}
		else if (std::isalnum(static_cast<unsigned char>(e)) ||
			e == '-' || e == '_' || e == '.' || e == '~')
		{
			res += e;
		}
		else
		{
			res += "%" + HexEncode(e);
		}
	}

	return res;
}

std::string Url::HexEncode(char const c)
{
	char s[3];

	if (c & 0x80)
	{
		std::snprintf(&s[0], 3, "%02X",
			static_cast<unsigned int>(c & 0xff)
		);
	}
	else
	{
		std::snprintf(&s[0], 3, "%02X",
			static_cast<unsigned int>(c)
		);
	}

	return std::string(s);
}

const std::string& Url::protocol()
{
	return protocol_;
}

const std::string& Url::domain()
{
	return domain_;
}

const std::string& Url::port()
{
	return port_;
}

const std::string& Url::path()
{
	return path_;
}

const std::string& Url::query()
{
	return query_;
}

bool Url::QueryValue(const std::string& key, std::string& value) const
{
	std::unordered_multimap<std::string, std::string>::const_iterator itr = queryKV_.find(key);
	if (itr != queryKV_.end())
	{
		value = itr->second;
		return true;
	}

	return false;
}