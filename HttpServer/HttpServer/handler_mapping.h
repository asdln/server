#ifndef HTTPSERVER_HANDLER_MAPPING_H_
#define HTTPSERVER_HANDLER_MAPPING_H_

#include <map>
#include <string>

#include "handler.h"

class HandlerMapping
{
public:

	Handler* GetHandler(const std::string& path);

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

#endif //HTTPSERVER_HANDLER_MAPPING_H_

