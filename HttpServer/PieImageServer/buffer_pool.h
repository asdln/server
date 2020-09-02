#ifndef HTTPSERVER_BUFFER_POOL_H_
#define HTTPSERVER_BUFFER_POOL_H_

#include <string>

class Buffer;

class BufferPool
{
public:

	Buffer* GetBuffer(const std::string& type, int size);
};

#endif //HTTPSERVER_BUFFER_POOL_H_
