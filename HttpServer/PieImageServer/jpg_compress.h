#ifndef HTTPSERVER_JPGCOMPRESS_H_
#define HTTPSERVER_JPGCOMPRESS_H_

#include "compress.h"

class JpgCompress :
    public Compress
{
public:

    virtual bool DoCompress(void* lpBmpBuffer, int nWidth, int nHeight, void** ppJpegBuffer, unsigned long& pOutSize) override;
};

#endif //HTTPSERVER_JPGCOMPRESS_H_