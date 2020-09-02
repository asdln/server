#include "jpg_buffer.h"
#include "stdlib.h"

JpgBuffer::~JpgBuffer()
{
	if (data_ != nullptr)
		free(data_);
}