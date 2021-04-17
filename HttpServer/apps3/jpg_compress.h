#pragma once

#include <vector>

class JpgCompress
{
public:

    void DoCompress(void* lpBmpBuffer, int nWidth, int nHeight, std::vector<unsigned char>& output);
};
