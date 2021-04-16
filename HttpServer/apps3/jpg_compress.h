#ifndef HTTPSERVER_JPGCOMPRESS_H_
#define HTTPSERVER_JPGCOMPRESS_H_

#include <vector>

class JpgCompress
{
public:

    void DoCompress(void* lpBmpBuffer, int nWidth, int nHeight, std::vector<unsigned char>& output);
};

#endif //HTTPSERVER_JPGCOMPRESS_H_