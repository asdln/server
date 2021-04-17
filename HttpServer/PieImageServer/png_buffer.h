#pragma once

#include <vector>
#include "buffer.h"
class PngBuffer :
    public Buffer
{
	friend class PngCompress;

public:

	virtual void* data() override;

	virtual size_t size() override;

    ~PngBuffer();

	std::vector<unsigned char>& GetVec();

protected:

	std::vector<unsigned char> data_;
};