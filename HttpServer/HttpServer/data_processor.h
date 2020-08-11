#pragma once

#include <string>

class DataProcessor
{
public:

	bool GetData(const std::string& target, void** pData, unsigned long& nDataBytes, std::string& mimeType);

	~DataProcessor();
	DataProcessor();

private:

	void* pDstBuffer_;
};

