#ifndef HTTPSERVER_JPGCOMPRESS_H_
#define HTTPSERVER_JPGCOMPRESS_H_

#include "compress.h"
#include <vector>

class JpgCompress :
    public Compress
{
public:

    virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) override;

    static void DoCompress(void* srcbuffer, int nSrcWidth, int nSrcHeight, std::vector<unsigned char>& output);
};

#endif //HTTPSERVER_JPGCOMPRESS_H_