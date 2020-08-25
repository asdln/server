#ifndef HTTPSERVER_STYLE_MANAGER_H_
#define HTTPSERVER_STYLE_MANAGER_H_

#include <string>
#include <map>
#include <memory>
#include <shared_mutex>
#include "style.h"


class StyleManager
{
public:

	static bool UpdateStyle(const std::string& jsonStyle, std::string& style_key);

	static std::shared_ptr<Style> GetStyle(const std::string& styleKey, size_t version);

	 
protected:

	static std::string GetStyleKey(Style* pStyle);

	static std::shared_ptr<Style> FromJson(const std::string& jsonStyle);

	static std::string ToJson(std::shared_ptr<Style> style);

	static std::shared_ptr<Style> GetFromStyleMap(const std::string& key);

	static bool AddOrUpdateStyleMap(const std::string& key, std::shared_ptr<Style> style);

protected:

	static std::map<std::string, std::shared_ptr<Style>> s_style_map_;

	static std::shared_mutex s_shared_mutex;
};

#endif //HTTPSERVER_STYLE_MANAGER_H_