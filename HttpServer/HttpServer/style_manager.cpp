#include "style_manager.h"
#include "true_color_style.h"

bool StyleManager::UpdateStyle(const std::string& jsonStyle)
{
	return true;
}

std::shared_ptr<Style> StyleManager::GetStyle(const std::string& kind)
{
	auto p = std::make_shared<TrueColorStyle>();
	return p;
}