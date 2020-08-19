#include "true_color_style.h"


std::shared_ptr<StretchInterface> TrueColorStyle::stretch()
{
	return stretch_;
}

void TrueColorStyle::set_stretch(std::shared_ptr<StretchInterface> stretch)
{
	stretch_ = stretch;
}

int TrueColorStyle::band_count()
{
	return band_count_;
}

void TrueColorStyle::set_band_count(int band_count)
{
	band_count_ = band_count;
}