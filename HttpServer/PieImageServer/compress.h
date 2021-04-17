#pragma once

#include "buffer.h"

class Compress
{
public:

	virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) = 0;
	
};
