#pragma once

#include "buffer.h"
class JpgBuffer :
    public Buffer
{
public:

	virtual void* data() override;

	virtual size_t size() override;

    void set_data(void* data, size_t size);

    ~JpgBuffer();

protected:

	void* data_ = nullptr;

	size_t size_ = 0;
};
