#ifndef PIEIMAGESERVER_WEBPCOMPRESS_H_
#define PIEIMAGESERVER_WEBPCOMPRESS_H_

#include "compress.h"

class WebpCompress : public Compress
{
public:

	virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) override;
};

#endif //PIEIMAGESERVER_WEBPCOMPRESS_H_