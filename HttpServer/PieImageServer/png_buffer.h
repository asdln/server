#ifndef PIEIMAGESERVER_BUFFER_H_
#define PIEIMAGESERVER_BUFFER_H_

#include <vector>
#include "buffer.h"
class PngBuffer :
    public Buffer
{
	friend class PngCompress;

public:

	virtual void* data() override;

	virtual size_t size() override;

    ~PngBuffer();

	std::vector<unsigned char>& GetVec();

protected:

	std::vector<unsigned char> data_;
};

#endif //PIEIMAGESERVER_BUFFER_H_