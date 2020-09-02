#include "buffer.h"

void* Buffer::data()
{
	return data_;
}

size_t Buffer::size()
{
	return size_;
}

void Buffer::set_data(void* data, size_t size)
{
	data_ = data;
	size_ = size;
}