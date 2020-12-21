#ifndef HTTPSERVER_STYLE_MANAGER_H_
#define HTTPSERVER_STYLE_MANAGER_H_

#include <string>
#include <map>
#include <memory>
#include <shared_mutex>
#include "style.h"
#include "dataset.h"

class StyleManager
{
public:

	// 根据style_string的内容获取StylePtr，可以节省序列化和style的prepare的时间。
	static StylePtr GetStyle(const std::string& style_string, DatasetPtr dataset);

protected:


protected:

	static std::map<std::string, StylePtr> s_map_style_container_;
	static std::shared_mutex s_shared_mutex_style_container_;
};

#endif //HTTPSERVER_STYLE_MANAGER_H_