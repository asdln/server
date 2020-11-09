#include "style.h"
#include "tiff_dataset.h"

Format default_format = Format::WEBP;
std::string default_string_format = "webp";

std::string StyleType2String(StyleType style_type)
{
	std::string str_style = "";

	switch (style_type)
	{
	case StyleType::TRUE_COLOR:
		str_style = "trueColor";
		break;
	case StyleType::DEM:
		str_style = "dem";
		break;
	case StyleType::GRAY:
		str_style = "gray";
		break;
	default:
		str_style = "none";
		break;
	}

	return str_style;
}

StyleType String2StyleType(const std::string& str_style)
{
	if (str_style.compare("trueColor") == 0)
	{
		return StyleType::TRUE_COLOR;
	}
	else if (str_style.compare("dem") == 0)
	{
		return StyleType::DEM;
	}
	else if (str_style.compare("gray") == 0)
	{
		return StyleType::GRAY;
	}

	return StyleType::NONE;
}

std::string Format2String(Format format)
{
	std::string string_fromat = "";
	switch (format)
	{
	case Format::JPG:
		string_fromat = "jpg";
		break;
	case Format::PNG:
		string_fromat = "png";
		break;
	case Format::WEBP:
		string_fromat = "webp";
		break;
	case Format::AUTO:
		string_fromat = "auto";
		break;
	default:
		string_fromat = default_string_format;
		break;
	}

	return string_fromat;
}

Format String2Format(const std::string& format_string)
{
	if (format_string.compare("jpg") == 0)
	{
		return Format::JPG;
	}
	else if (format_string.compare("png") == 0)
	{
		return Format::PNG;
	}
	else if (format_string.compare("webp") == 0)
	{
		return Format::WEBP;
	}
	else if (format_string.compare("auto") == 0)
	{
		return Format::AUTO;
	}
	else
	{
		return default_format;
	}
}

StylePtr Style::Clone() 
{
	StylePtr pClone = std::make_shared<Style>();
	pClone->uid_ = uid_;
	pClone->version_ = version_;
	pClone->format_ = format_;
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;
	pClone->stretch_ = stretch_->Clone();
	return pClone;
}

StylePtr Style::CompletelyClone()
{
	StylePtr pClone = std::make_shared<Style>();
	pClone->uid_ = uid_;
	pClone->version_ = version_;
	pClone->format_ = format_;
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;
	pClone->stretch_ = stretch_->Clone();
	pClone->stretch_->need_refresh_ = stretch_->need_refresh_; //完全拷贝，"刷新标记"也拷贝
	return pClone;
}

void Style::Prepare(DatasetPtr dataset)
{
	std::shared_ptr<TiffDataset> tiffDataset = std::dynamic_pointer_cast<TiffDataset>(dataset);
	int dataset_band_count = tiffDataset->GetBandCount();
	int nBandCount = bandCount_;
	int bandMap[4] = { bandMap_[0] <= dataset_band_count ? bandMap_[0] : dataset_band_count
		, bandMap_[1] <= dataset_band_count ? bandMap_[1] : dataset_band_count
		, bandMap_[2] <= dataset_band_count ? bandMap_[2] : dataset_band_count
		, bandMap_[3] <= dataset_band_count ? bandMap_[3] : dataset_band_count };


	if (format_ == Format::JPG)
		nBandCount = 3;

	if (nBandCount != 3 && nBandCount != 4)
	{
		nBandCount = 3;
	}

	bandCount_ = nBandCount;
	for (int i = 0; i < bandCount_; i++)
	{
		bandMap_[i] = bandMap[i];
	}

	stretch_->Prepare(bandCount_, bandMap_, dataset.get());
}