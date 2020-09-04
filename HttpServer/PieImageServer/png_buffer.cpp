#include "png_buffer.h"


PngBuffer::~PngBuffer()
{
	
}

void* PngBuffer::data() 
{
	return data_.data();
}

size_t PngBuffer::size()
{
	return data_.size();
}

std::vector<unsigned char>& PngBuffer::GetVec()
{
	return data_;
}