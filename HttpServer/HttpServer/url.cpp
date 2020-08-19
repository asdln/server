#include "url.h"

#include <string>
#include <iostream>
#include <boost/regex.hpp>
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
					queryKV_.emplace(keyValue[0], keyValue[1]);
				}
			}
		}
	}
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
	std::map<std::string, std::string>::const_iterator itr = queryKV_.find(key);
	if (itr != queryKV_.end())
	{
		value = itr->second;
		return true;
	}

	return false;
}