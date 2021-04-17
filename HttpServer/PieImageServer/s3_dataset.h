#pragma once

#include "tiff_dataset.h"

class S3Dataset : public TiffDataset
{
public:

	virtual bool Open(const std::string& path) override;

	virtual bool Read(int nx, int ny, int width, int height,
		void* pData, int bufferWidth, int bufferHeight, DataType dataType,
		int nBandCount, int* pBandMap, long long pixSpace = 0, long long lineSapce = 0, long long bandSpace = 0,
		void* psExtraArg = nullptr) override;

protected:

	std::vector<std::pair<size_t, size_t>> dims_;

	int type_bytes_ = 1;
};