#ifndef HTTPSERVER_COMPRESSINTERFACE_H_
#define HTTPSERVER_COMPRESSINTERFACE_H_


class Compress
{
public:

	virtual bool DoCompress(void* lpBmpBuffer, int nWidth, int nHeight, void** ppJpegBuffer, unsigned long& pOutSize) = 0;
	
};

#endif // !HTTPSERVER_COMPRESSINTERFACE_H_