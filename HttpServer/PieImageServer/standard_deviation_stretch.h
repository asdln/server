#pragma once

#include "min_max_stretch.h"
#include "math.h"

class StandardDeviationStretch : public MinMaxStretch
{
public:

	StandardDeviationStretch();
	~StandardDeviationStretch() {}

	void set_dev_scale(double dev_scale) { dev_scale_ = dev_scale; }

	double dev_scale() { return dev_scale_; }

	virtual void Prepare(int band_count, int* band_map, Dataset* dataset) override;

	StretchPtr Clone() override;

protected:

	void Copy(StandardDeviationStretch*);

	double ChangeDataRange(double value, DataType dt);

protected:

	double dev_scale_ = 2.05;
};

typedef std::shared_ptr<StandardDeviationStretch> StandardDeviationStretchPtr;