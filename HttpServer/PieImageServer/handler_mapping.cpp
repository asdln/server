#include "handler_mapping.h"
#include "wmts_handler.h"


HandlerMapping* HandlerMapping::instance_ = nullptr;
std::mutex HandlerMapping::mutex_;


HandlerMapping::HandlerMapping()
{
	RegisterAll();
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
	if(instance_ != nullptr)
		delete instance_;

	instance_ = nullptr;
}

void HandlerMapping::RegisterAll()
{
	Handler* handler = new WMTSHandler;
	handlerMap_.insert(std::make_pair("wmts", handler));
}

Handler* HandlerMapping::GetHandler(Url& url)
{
	std::string service = "";
	if (!url.QueryValue("service", service))
	{
		if (!url.QueryValue("Service", service))
		{
			url.QueryValue("SERVICE", service);
		}
	}

	std::map<std::string, Handler*>::iterator iter = handlerMap_.find(service);
	if (iter != handlerMap_.end())
	{
		return iter->second;
	}
	else
	{
		return handlerMap_["wmts"];
	}
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