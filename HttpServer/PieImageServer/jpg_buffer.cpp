#include "jpg_buffer.h"
#include "stdlib.h"

JpgBuffer::~JpgBuffer()
{
	if (data_ != nullptr)
		free(data_);
}

void JpgBuffer::set_data(void* data, size_t size)
{
	data_ = data;
	size_ = size;
}

void* JpgBuffer::data()
{
	return data_;
}

size_t JpgBuffer::size()
{
	return size_;
}