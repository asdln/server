#pragma once

#include "compress.h"

class PngCompress : public Compress
{
public:

	virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) override;
};