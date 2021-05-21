#include "memory_pool.h"

void MemoryPoolTest()
{
	MemoryPool* mem_pool_ = new MemoryPool;
	unsigned char* buffer1 = mem_pool_->malloc(256 * 4);
	unsigned char* buffer2 = mem_pool_->malloc(256 * 4);
	unsigned char* buffer3 = mem_pool_->malloc(256 * 4);
	unsigned char* buffer4 = mem_pool_->malloc(256 * 4);
	unsigned char* buffer5 = mem_pool_->malloc(256 * 4);

	mem_pool_->free(buffer1);
	mem_pool_->free(buffer5);
	mem_pool_->free(buffer2);

	unsigned char* buffer6 = mem_pool_->malloc(128 * 4);
	unsigned char* buffer7 = mem_pool_->malloc(128 * 4);

	unsigned char* buffer8 = mem_pool_->malloc(256 * 5);

	delete mem_pool_;
}

unsigned char* MemoryPool::malloc(size_t length)
{
	if (mem_tag_.empty())
	{
		unsigned char* buffer = new unsigned char[length];
		//mem_blocks_.insert({ buffer, {buffer, length} });
		mem_blocks_.emplace(std::piecewise_construct, std::forward_as_tuple(buffer), std::forward_as_tuple(buffer, length));
		return buffer;
	}
	else
	{
		for (std::list<unsigned char*>::iterator itr = mem_tag_.begin(); itr != mem_tag_.end(); itr++)
		{
			std::unordered_map<unsigned char*, MemoryBlock>::iterator itr_map = mem_blocks_.find(*itr);
			if (itr_map != mem_blocks_.end() && itr_map->second.length >= length)
			{
				itr_map->second.used = true;
				mem_tag_.erase(itr);
				return itr_map->first;
			}
		}

		unsigned char* buffer = new unsigned char[length];
		mem_blocks_.emplace(std::piecewise_construct, std::forward_as_tuple(buffer), std::forward_as_tuple(buffer, length));
		return buffer;
	}
}

void MemoryPool::free(unsigned char* buffer)
{
	std::unordered_map<unsigned char*, MemoryBlock>::iterator itr = mem_blocks_.find(buffer);
	if (itr != mem_blocks_.end())
	{
		itr->second.used = false;
	}

	mem_tag_.push_back(buffer);
}