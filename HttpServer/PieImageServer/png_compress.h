#ifndef PIEIMAGESERVER_JPGCOMPRESS_H_
#define PIEIMAGESERVER_JPGCOMPRESS_H_

#include "compress.h"

class PngCompress : public Compress
{
public:

	virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) override;
};

#endif //PIEIMAGESERVER_JPGCOMPRESS_H_