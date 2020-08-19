#ifndef HTTPSERVER_BUFFER_H_
#define HTTPSERVER_BUFFER_H_


class Buffer
{
public:

	virtual void* data();

	virtual size_t size();

	virtual void set_data(void* data, size_t size);

	virtual ~Buffer(){}

protected:

	void* data_ = nullptr;

	size_t size_ = 0;

};

#endif //HTTPSERVER_BUFFER_H_