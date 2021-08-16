#pragma once

#include <unordered_map>
#include <shared_mutex>

class StyleMap
{

public:

	static void StartStyleWatch();

	static void Update(const std::string& key, const std::string& value);

	static bool Find(const std::string& key, std::string& value);

public:

	static std::shared_mutex s_mutex_;

	static std::unordered_map<std::string, std::string> style_map_;

	static std::string style_map_prefix_;
};

