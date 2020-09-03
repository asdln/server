#ifndef HTTPSERVER_BUFFER_H_
#define HTTPSERVER_BUFFER_H_

#include <memory>

class Buffer
{
public:

	virtual void* data() = 0;

	virtual size_t size() = 0;

	virtual ~Buffer(){}

protected:



};

typedef std::shared_ptr<Buffer> BufferPtr;

#endif //HTTPSERVER_BUFFER_H_