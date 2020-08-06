#ifndef HTTPSERVER_JPGCOMPRESS_H_
#define HTTPSERVER_JPGCOMPRESS_H_

#include "compress_interface.h"

class JpgCompress :
    public CompressInterface
{
public:

    virtual bool Compress(void* lpBmpBuffer, int nWidth, int nHeight, void** ppJpegBuffer, unsigned long* pOutSize) override;
};

#endif //HTTPSERVER_JPGCOMPRESS_H_