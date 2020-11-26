#ifndef HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_
#define HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_

#include "min_max_stretch.h"

class Dataset;

class HistogramEqualizeStretch :
	public MinMaxStretch
{
public:

	void DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset) override;

	void set_stretch_percent(double stretch_percent) { stretch_percent_ = stretch_percent; }

	double stretch_percent() { return stretch_percent_; }

	virtual void Prepare(int band_count, int* band_map, Dataset* dataset) override;

	StretchPtr Clone() override;

protected:

	void Copy(HistogramEqualizeStretch*);

	void HistogramEqualize(double* pdHistogram, int nBandIndex);

protected:

	double stretch_percent_ = 0.0;

	unsigned char lut_[4][256];
};

typedef std::shared_ptr<HistogramEqualizeStretch> HistogramEqualizeStretchPtr;

#endif  //HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_