#ifndef HTTPSERVER_COMPRESSINTERFACE_H_
#define HTTPSERVER_COMPRESSINTERFACE_H_


#include "buffer.h"

class Compress
{
public:

	virtual BufferPtr DoCompress(void* lpBmpBuffer, int nWidth, int nHeight) = 0;
	
};

#endif // !HTTPSERVER_COMPRESSINTERFACE_H_