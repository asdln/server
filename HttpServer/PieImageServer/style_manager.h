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

	// ����style_string�����ݻ�ȡStylePtr�����Խ�ʡ���л���style��prepare��ʱ�䡣
	static StylePtr GetStyle(const std::string& style_string, DatasetPtr dataset);

protected:


protected:

	static std::map<std::string, StylePtr> s_map_style_container_;
	static std::shared_mutex s_shared_mutex_style_container_;
};

#endif //HTTPSERVER_STYLE_MANAGER_H_