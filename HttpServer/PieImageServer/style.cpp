#include "style.h"
#include "tiff_dataset.h"
#include "true_color_style.h"
#include "min_max_stretch.h"
#include "histogram_equalize_stretch.h"
#include "standard_deviation_stretch.h"
#include "percent_min_max_stretch.h"
#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"
#include "gdal_priv.h"

#include <set>

Format default_format = Format::WEBP;
std::string default_string_format = "webp";

void ColorInterpolation(int index1, int index2, unsigned char charcolor[][4])
{
	int count = index2 - index1 - 1;
	if (count <= 0)
		return;

	const unsigned char& r1 = charcolor[index1][0];
	const unsigned char& g1 = charcolor[index1][1];
	const unsigned char& b1 = charcolor[index1][2];
	const unsigned char& a1 = charcolor[index1][3];

	const unsigned char& r2 = charcolor[index2][0];
	const unsigned char& g2 = charcolor[index2][1];
	const unsigned char& b2 = charcolor[index2][2];
	const unsigned char& a2 = charcolor[index2][3];

	float step_r = (r2 - r1) / float(count + 1);
	float step_g = (g2 - g1) / float(count + 1);
	float step_b = (b2 - b1) / float(count + 1);
	float step_a = (a2 - a1) / float(count + 1);

	for (int i = 0; i < count; i ++)
	{
		charcolor[index1 + 1 + i][0] = r1 + i * step_r;
		charcolor[index1 + 1 + i][1] = g1 + i * step_g;
		charcolor[index1 + 1 + i][2] = b1 + i * step_b;
		charcolor[index1 + 1 + i][3] = a1 + i * step_a;
	}
}

std::string StyleType2String(StyleType style_type)
{
	std::string str_style = "";

	switch (style_type)
	{
	case StyleType::TRUE_COLOR:
		str_style = "trueColor";
		break;
	case StyleType::PALETTE:
		str_style = "palette";
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
	else if (str_style.compare("palette") == 0)
	{
		return StyleType::PALETTE;
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
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;

	pClone->stretch_ = nullptr;

	memcpy(pClone->lut_, lut_, sizeof(lut_));

	if (stretch_)
	{
		pClone->stretch_ = stretch_->Clone();
	}
	
	return pClone;
}

StylePtr Style::CompletelyClone()
{
	StylePtr pClone = std::make_shared<Style>();
	pClone->uid_ = uid_;
	pClone->version_ = version_;
	pClone->kind_ = kind_;
	for (int i = 0; i < 4; i++)
	{
		pClone->bandMap_[i] = bandMap_[i];
	}
	pClone->bandCount_ = bandCount_;
	pClone->srs_epsg_code_ = srs_epsg_code_;

	pClone->stretch_ = nullptr;

	memcpy(pClone->lut_, lut_, sizeof(lut_));

	if (stretch_)
	{
		pClone->stretch_ = stretch_->Clone();
		pClone->stretch_->need_refresh_ = stretch_->need_refresh_; //完全拷贝，"刷新标记"也拷贝
	}
		
	return pClone;
}

void Style::Prepare(Dataset* dataset)
{
	TiffDataset* tiffDataset = dynamic_cast<TiffDataset*>(dataset);
	int dataset_band_count = tiffDataset->GetBandCount();
	int nBandCount = bandCount_;
	int bandMap[4] = { bandMap_[0] <= dataset_band_count ? bandMap_[0] : dataset_band_count
		, bandMap_[1] <= dataset_band_count ? bandMap_[1] : dataset_band_count
		, bandMap_[2] <= dataset_band_count ? bandMap_[2] : dataset_band_count
		, bandMap_[3] <= dataset_band_count ? bandMap_[3] : dataset_band_count };

	if (kind_ == StyleType::TRUE_COLOR)
	{
		if (nBandCount != 3 && nBandCount != 4)
		{
			nBandCount = 3;
		}
	}

	bandCount_ = nBandCount;
	for (int i = 0; i < bandCount_; i++)
	{
		bandMap_[i] = bandMap[i];
	}

	if(stretch_)
		stretch_->Prepare(bandCount_, bandMap_, dataset);
}

void Style::Apply(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset, int render_color_count)
{
	unsigned char* buffer = (unsigned char*)data;

	//色彩映射表不做拉伸
	if (kind_ != StyleType::PALETTE && stretch_ != nullptr)
	{
		stretch_->DoStretch(data, mask_buffer, size, band_count, band_map, dataset);

		if (kind_ == StyleType::TRUE_COLOR)
		{
			//根据mask的值，添加透明通道的值。
			if (render_color_count == 4 && band_count == 3)
			{
				for (int j = size - 1; j >= 0; j--)
				{
					int srcIndex = j * 3;
					int dstIndex = j * 4;

					memcpy(buffer + dstIndex, buffer + srcIndex, 3);
					buffer[dstIndex + 3] = mask_buffer[j];
				}
			}
		}
		else if (kind_ == StyleType::DEM)
		{
			bool band_4 = render_color_count == 4;

			for (int j = size - 1; j >= 0; j--)
			{
				int dstIndex = j * render_color_count;
				if (mask_buffer[j] != 0)
				{
					int srcIndex = j;

					//因为是倒序覆盖，所以重写索引要从大到小
					if (band_4)
						buffer[dstIndex + 3] = lut_[buffer[srcIndex]][3];

					buffer[dstIndex + 2] = lut_[buffer[srcIndex]][2];
					buffer[dstIndex + 1] = lut_[buffer[srcIndex]][1];
					buffer[dstIndex] = lut_[buffer[srcIndex]][0];
				}
				else
				{
					if (band_4)
						buffer[dstIndex + 3] = 0;

					buffer[dstIndex + 2] = 255;
					buffer[dstIndex + 1] = 255;
					buffer[dstIndex] = 255;
				}
			}
		}
	}
	else if (kind_ == StyleType::PALETTE)
	{
		bool band_4 = render_color_count == 4;
		GDALColorTable* poColorTable = dataset->GetColorTable(1);
		if (poColorTable && dataset->GetDataType() == DT_Byte)
		{
			for (int i = size - 1; i >= 0; i--)
			{
				if (mask_buffer[i] != 0)
				{
					const GDALColorEntry* poColorEntry = poColorTable->GetColorEntry(buffer[i]);
					if(poColorEntry == nullptr)
						continue;

					if (band_4)
						buffer[i * render_color_count + 3] = poColorEntry->c4;

					buffer[i * render_color_count + 2] = poColorEntry->c3;
					buffer[i * render_color_count + 1] = poColorEntry->c2;
					buffer[i * render_color_count] = poColorEntry->c1;
				}
				else
				{
					if (band_4)
						buffer[i * render_color_count + 3] = 0;

					buffer[i * render_color_count + 2] = 255;
					buffer[i * render_color_count + 1] = 255;
					buffer[i * render_color_count] = 255;
				}
			}
		}
	}
}

void Style::set_band_map(int* band_map, int band_count)
{
	bandCount_ = band_count;

	for(int i = 0; i < band_count; i ++)
	{
		bandMap_[i] = band_map[i];
	}
}

void Style::init_lut()
{
	memset(lut_, 255, sizeof(lut_));

	if (kind_ == StyleType::DEM)
	{
		bandCount_ = 1;
	}

	lut_[0][0] = 0;
	lut_[0][1] = 0;
	lut_[0][2] = 0;
	lut_[0][3] = 255;

	lut_[255][0] = 255;
	lut_[255][1] = 255;
	lut_[255][2] = 255;
	lut_[255][3] = 255;

	ColorInterpolation(0, 255, lut_);
}

StylePtr StyleSerielizer::FromJsonObj(neb::CJsonObject& json_obj)
{
	StylePtr style;
	style = std::make_shared<Style>();
	if (json_obj.Get("bandCount", style->bandCount_))
	{
		for (int i = 0; i < style->bandCount_; i++)
		{
			json_obj["bandMap"].Get(i, style->bandMap_[i]);
		}
	}
	else
	{
		neb::CJsonObject json_band_map;
		json_obj.Get("bandMap", json_band_map);

		if (json_band_map.IsArray())
		{
			style->bandCount_ = json_band_map.GetArraySize();
			for (int i = 0; i < style->bandCount_; i++)
			{
				json_band_map.Get(i, style->bandMap_[i]);
			}
		}
	}

	json_obj.Get("version", style->version_);

	style->kind_ = String2StyleType(json_obj("kind"));
	if (style->kind_ == StyleType::NONE)
	{
		style->kind_ = StyleType::TRUE_COLOR;
	}

	//如果是truecolor，则band_count只能是3或者4
	if (style->kind_ == StyleType::TRUE_COLOR && style->bandCount_ != 3 && style->bandCount_ != 4)
	{
		style->bandCount_ = 3;
	}
	else if (style->kind_ == StyleType::DEM)
	{
		memset(style->lut_, 255, sizeof(style->lut_));
		style->bandCount_ = 1;
	}
	else if (style->kind_ == StyleType::PALETTE)
	{
		style->bandCount_ = 1;
	}

	style->uid_ = json_obj("uid");

	std::string stretch_kind = json_obj["stretch"]("kind");
	if (stretch_kind.compare(StretchType2String(StretchType::MINIMUM_MAXIMUM)) == 0)
	{
		double dMin = 0.0;
		double dMax = 0.0;

		auto stretch = std::make_shared<MinMaxStretch>();
		style->stretch_ = stretch;

		neb::CJsonObject json_min;
		neb::CJsonObject json_max;

		json_obj["stretch"].Get("minimum", json_min);
		json_obj["stretch"].Get("maximum", json_max);

		for (int i = 0; i < style->bandCount_; i++)
		{
			json_min.Get(i, stretch->min_value_[i]);
			json_max.Get(i, stretch->max_value_[i]);

			//json_obj["stretch"]["minimum"].Get(i, stretch->min_value_[i]);
			//json_obj["stretch"]["maximum"].Get(i, stretch->max_value_[i]);
		}
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::PERCENT_MINMAX)) == 0)
	{
		auto stretch = std::make_shared<PercentMinMaxStretch>();
		style->stretch_ = stretch;

		double percent = 1.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::HISTOGRAMEQUALIZE)) == 0)
	{
		auto stretch = std::make_shared<HistogramEqualizeStretch>();
		style->stretch_ = stretch;

		double percent = 0.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}
	else if (stretch_kind.compare(StretchType2String(StretchType::STANDARD_DEVIATION)) == 0)
	{
		auto stretch = std::make_shared<StandardDeviationStretch>();
		style->stretch_ = stretch;

		double scale = 2.5;
		json_obj["stretch"].Get("scale", scale);
		stretch->set_dev_scale(scale);
	}
	else if(style->kind_ != StyleType::PALETTE) //颜色映射表不做拉伸
	{
		auto stretch = std::make_shared<PercentMinMaxStretch>();
		style->stretch_ = stretch;

		double percent = 0.0;
		json_obj["stretch"].Get("percent", percent);
		stretch->set_stretch_percent(percent);
	}

	double external_nodata_value;
	if (json_obj["stretch"].Get("externalNodataValue", external_nodata_value))
	{
		style->get_stretch()->SetUseExternalNoDataValue(true);
		style->get_stretch()->SetExternalNoDataValue(external_nodata_value);
	}

	if (style->kind_ == StyleType::DEM)
	{
		neb::CJsonObject json_lut;
		json_obj.Get("lut", json_lut);

		style->lut_[0][0] = 0;
		style->lut_[0][1] = 0;
		style->lut_[0][2] = 0;
		style->lut_[0][3] = 255;

		style->lut_[255][0] = 255;
		style->lut_[255][1] = 255;
		style->lut_[255][2] = 255;
		style->lut_[255][3] = 255;

		//首尾默认添加
		std::set<int> tag_set;
		tag_set.emplace(0);
		tag_set.emplace(255);

		if (json_lut.IsArray())
		{
			int array_size = json_lut.GetArraySize();
			for (int i = 0; i < array_size; i ++)
			{
				neb::CJsonObject json_lut_entry;
				json_lut.Get(i, json_lut_entry);

				if (json_lut_entry.IsArray())
				{
					int lut_index = 0;
					int entry_size = json_lut_entry.GetArraySize();
					json_lut_entry.Get(0, lut_index);

					if(lut_index < 0 || lut_index >= 256)
						break;

					for (int j = 1; j < entry_size; j ++)
					{
						if(j >= 4)
							break;

						tag_set.emplace(lut_index);

						int color_entry = 255;
						json_lut_entry.Get(j, color_entry);
						style->lut_[lut_index][j - 1] = color_entry;
					}
				}
			}
		}

		//根据tag_set进行插值
		for (std::set<int>::iterator itr = tag_set.begin(); itr != tag_set.end(); itr++)
		{
			std::set<int>::iterator itr_next = itr;
			itr_next++;

			if (itr_next == tag_set.end())
				break;

			ColorInterpolation(*itr, *itr_next, style->lut_);
		}
	}

	return style;
}

StylePtr StyleSerielizer::FromJson(const std::string& jsonStyle)
{
	if (jsonStyle.empty())
		return nullptr;

	StylePtr style;

	try
	{
		neb::CJsonObject oJson(jsonStyle);
		neb::CJsonObject oJson_style;

		//如果不包含"style"，则认为已经提取了字段值
		if (!oJson.Get("style", oJson_style))
		{
			oJson_style = neb::CJsonObject(jsonStyle);
		}

		style = FromJsonObj(oJson_style);
	}
	catch (...)
	{
		LOG(ERROR) << "style format wrong";
	}

	return style;
}

//std::string StyleSerielizer::ToJson(StylePtr style)
//{
//	neb::CJsonObject oJson;
	//oJson.Add("uid", style->uid_.c_str());
	//oJson.Add("version", style->version_);
	//oJson.Add("kind", StyleType2String(style->kind_));
	//oJson.Add("format", Format2String(style->format_));
	//oJson.Add("bandCount", style->bandCount_);
	//oJson.AddEmptySubArray("bandMap");

	//for (int i = 0; i < style->bandCount_; i ++)
	//{
	//	oJson["bandMap"].Add(style->bandMap_[i]);
	//}

	//oJson.AddEmptySubObject("stretch");
	//if (style->stretch_->kind() == StretchType::MINIMUM_MAXIMUM)
	//{
	//	oJson["stretch"].Add("kind", StretchType2String(StretchType::MINIMUM_MAXIMUM));
	//	oJson["stretch"].Add("minimum", 0.0);
	//	oJson["stretch"].Add("maximum", 255.0);
	//}
	//else if (style->stretch_->kind() == StretchType::PERCENT_MINMAX)
	//{
	//	PercentMinMaxStretchPtr percent_min_max_stretch = std::dynamic_pointer_cast<PercentMinMaxStretch>(style);
	//	oJson["stretch"].Add("kind", StretchType2String(StretchType::PERCENT_MINMAX));
	//	oJson["stretch"].Add("percent", percent_min_max_stretch->stretch_percent());
	//}
	//else if (style->stretch_->kind() == StretchType::HISTOGRAMEQUALIZE)
	//{
	//}

// 	neb::CJsonObject oJson1;
//	oJson1.Add("style", oJson);

//	return oJson1.ToString();
//}