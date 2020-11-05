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

	static StylePtr GetStyle(const std::string& styleKey, size_t version);

	static StylePtr FromJson(const std::string& jsonStyle);

	static std::string ToJson(StylePtr style);

	 
protected:

	static std::string GetStyleKey(Style* pStyle);

	static StylePtr GetFromStyleMap(const std::string& key);

	static bool AddOrUpdateStyleMap(const std::string& key, StylePtr style);

protected:

	static std::map<std::string, StylePtr> s_style_map_;

	static std::shared_mutex s_shared_mutex;
};

#endif //HTTPSERVER_STYLE_MANAGER_H_