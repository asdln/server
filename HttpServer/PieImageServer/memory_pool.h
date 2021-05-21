#pragma once

#include <list>
#include <unordered_map>

struct MemoryBlock
{
	MemoryBlock(unsigned char* buf, size_t len)
	{
		buffer = buf;
		length = len;
	}

	~MemoryBlock()
	{
		if (buffer != nullptr)
			delete[] buffer;
	}

	unsigned char* buffer = nullptr;

	size_t length = 0;

	bool used = true;
};

class MemoryPool
{
public:

	unsigned char* malloc(size_t length);

	void free(unsigned char* buffer);

protected:

	std::unordered_map<unsigned char*, MemoryBlock> mem_blocks_;
	std::list<unsigned char*> mem_tag_;
	//std::list<MemoryBlock> mem_blocks_;
};

void MemoryPoolTest();