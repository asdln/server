#pragma once

#include <map>
#include <string>
#include <mutex>

#include "url.h"

class Handler;

class HandlerMapping
{
public:

	Handler* GetHandler(const Url& url);

	void RegisterAll();

	static HandlerMapping* GetInstance();

	static void DestroyInstance();

private:

	HandlerMapping();
	virtual ~HandlerMapping();
	HandlerMapping(const HandlerMapping&);
	HandlerMapping& operator = (const HandlerMapping&);

private:

	std::map<std::string, Handler*> handlerMap_;

	static HandlerMapping* instance_;

	static std::mutex mutex_;
};

