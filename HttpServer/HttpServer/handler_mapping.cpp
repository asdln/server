#include "handler_mapping.h"
#include "wmts_handler.h"


HandlerMapping* HandlerMapping::instance_ = nullptr;
std::mutex HandlerMapping::mutex_;


HandlerMapping::HandlerMapping()
{

}

HandlerMapping::~HandlerMapping()
{
	for (auto& val : handlerMap_)
	{
		delete val.second;
		val.second = nullptr;
	}
}

void HandlerMapping::DestroyInstance()
{
	if(instance_ == nullptr)
		delete instance_;

	instance_ = nullptr;
}

void HandlerMapping::RegisterAll()
{
	Handler* handler = new WMTSHandler;
	handlerMap_.insert(std::make_pair("wmts", handler));
}

Handler* HandlerMapping::GetHandler(const std::string& path)
{
	return handlerMap_.at(path);
}

HandlerMapping* HandlerMapping::GetInstance()
{
	if (instance_ == nullptr)
	{
		std::lock_guard<std::mutex> guard(mutex_);
		{
			if (instance_ == nullptr)
			{
				instance_ = new HandlerMapping;
			}
		}
	}

	return instance_;
}