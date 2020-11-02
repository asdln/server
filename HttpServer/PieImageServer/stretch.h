#ifndef HTTPSERVER_STRETCH_INTERFACE_H_
#define HTTPSERVER_STRETCH_INTERFACE_H_

#include <memory>
#include <mutex>
#include "utility.h"
#include "histogram.h"

enum class StretchType
{
	MINIMUM_MAXIMUM,
	PERCENT_MINMAX
};

class Stretch
{
public: 
	virtual void DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset) {};

	virtual StretchType kind() { return kind_; }

	virtual void Refresh() { need_refresh_ = true; }

	//使用外部传入的无效值
	virtual void SetUseExternalNoDataValue(bool useExternal);

	virtual bool GetUseExternalNoDataValue();

	virtual void SetExternalNoDataValue(double useExternal);

	virtual double GetExternalNoDataValue();

protected:

	bool need_refresh_ = true;

	StretchType kind_;

	std::mutex mutex_;

	bool use_external_nodata_value_ = false;

	double external_nodata_value_ = 0.0;
};

std::string StretchType2String(StretchType type);
StretchType String2StretchType(std::string type_string);

#endif HTTPSERVER_STRETCH_INTERFACE_H_