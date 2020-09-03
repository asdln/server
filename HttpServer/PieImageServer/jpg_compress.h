#ifndef HTTPSERVER_JPGCOMPRESS_H_
#define HTTPSERVER_JPGCOMPRESS_H_

#include "compress.h"

class JpgCompress :
    public Compress
{
public:

    virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) override;
};

#endif //HTTPSERVER_JPGCOMPRESS_H_