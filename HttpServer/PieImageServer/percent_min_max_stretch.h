#ifndef HTTPSERVER_PERCENT_MIN_MAX_STRETCH_H_
#define HTTPSERVER_PERCENT_MIN_MAX_STRETCH_H_

#include "min_max_stretch.h"

class PercentMinMaxStretch : public MinMaxStretch
{
public:

	PercentMinMaxStretch();

	virtual void Prepare(int band_count, int* band_map, Dataset* dataset) override;

	void set_stretch_percent(double stretch_percent) { stretch_percent_ = stretch_percent; }

	double stretch_percent() { return stretch_percent_; }

	StretchPtr Clone() override;

protected:

	void Copy(PercentMinMaxStretch*);

protected:

	double stretch_percent_ = 0.0;
};

typedef std::shared_ptr<PercentMinMaxStretch> PercentMinMaxStretchPtr;


#endif //HTTPSERVER_PERCENT_MIN_MAX_STRETCH_H_
