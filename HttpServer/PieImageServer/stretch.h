#ifndef HTTPSERVER_STRETCH_INTERFACE_H_
#define HTTPSERVER_STRETCH_INTERFACE_H_

#include <memory>
#include <mutex>
#include "utility.h"
#include "histogram.h"

enum class StretchType
{
	MINIMUM_MAXIMUM,
	PERCENT_MINMAX,
	HISTOGRAMEQUALIZE,
	STANDARD_DEVIATION
};

class Stretch;
typedef std::shared_ptr<Stretch> StretchPtr;

class Stretch
{
	friend class Style;
public: 
	virtual void DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset) {};

	virtual void Prepare(int band_count, int* band_map, Dataset* dataset) = 0;

	virtual StretchType kind() { return kind_; }

	virtual void Refresh() { need_refresh_ = true; }

	//ʹ���ⲿ�������Чֵ
	virtual void SetUseExternalNoDataValue(bool useExternal);

	virtual bool GetUseExternalNoDataValue();

	virtual void SetExternalNoDataValue(double useExternal);

	virtual double GetExternalNoDataValue();

	virtual StretchPtr Clone() = 0;

protected:

	void Copy(Stretch*);

protected:

	//style��styleManager��Ļ����ȡ���������������ȡ�������Ѿ����������ݵģ���ȫclone�ģ�����Ҫ��refresh��
	bool need_refresh_ = true;

	StretchType kind_;

	//std::mutex mutex_;

	bool use_external_nodata_value_ = false;

	double external_nodata_value_ = 0.0;
};



std::string StretchType2String(StretchType type);
StretchType String2StretchType(std::string type_string);

#endif HTTPSERVER_STRETCH_INTERFACE_H_