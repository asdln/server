#include "style_manager.h"
#include "true_color_style.h"
#include "min_max_stretch.h"
#include "histogram_equalize_stretch.h"
#include "standard_deviation_stretch.h"
#include "etcd_storage.h"
#include "percent_min_max_stretch.h"

#include "utility.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

std::map<std::string, StylePtr> StyleManager::s_map_style_container_;
std::shared_mutex StyleManager::s_shared_mutex_style_container_;

//示例：
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "minimumMaximum", "minimum" : [0.0, 0.0, 0.0] , "maximum" : [255.0, 255.0, 255.0] }}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "histogramEqualize", "percent" : 0.0}}}
//{"style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind": "standardDeviation", "scale" : 2.05}}}

//可以省略一些字段，例如：
//{"style":{"stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}}

//post body:

//例子1: {"info":[{"path":"d:/1.tif","style":{"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}},{"path":"d:/2.tif", "style" : {"kind":"trueColor", "bandMap" : [1, 2, 3] , "bandCount" : 3, "stretch" : {"kind":"percentMinimumMaximum", "percent" : 3.0}}}]}
//例子2: {"info":[{"path":"d:/1.tif","style":{"stretch":{"kind":"percentMinimumMaximum","percent":3.0}}},{"path":"d:/2.tif","style":{"stretch":{"kind":"percentMinimumMaximum","percent":3.0}}}]}
//例子3: {"info":[{"path":"d:/linux_share/DEM-Gloable32.tif","style":{"stretch":{"kind":"standardDeviation","scale":0.5}}},{"path":"d:/linux_share/t/1.tiff","style":{"stretch":{"kind":"percentMinimumMaximum","percent":3.0}}}]}


StylePtr StyleManager::GetStyle(const std::string& style_string, DatasetPtr dataset)
{
	std::string file_path = dataset->file_path();
	std::string string_style;
	neb::CJsonObject oJson_style;

	string_style = style_string;
	
	//if (!string_style.empty())
	{
		file_path += string_style;
		std::string md5;
		GetMd5(md5, file_path.c_str(), file_path.size());

		{
			std::shared_lock<std::shared_mutex> lock(s_shared_mutex_style_container_);
			std::map<std::string, StylePtr>::iterator itr;

			itr = s_map_style_container_.find(md5);
			if (itr != s_map_style_container_.end())
			{
				StylePtr style = itr->second->CompletelyClone();
				return style;
			}
		}

		{
			std::unique_lock<std::shared_mutex> lock(s_shared_mutex_style_container_);
			std::map<std::string, StylePtr>::iterator itr;

			itr = s_map_style_container_.find(md5);
			if (itr != s_map_style_container_.end())
			{
				StylePtr style = itr->second->CompletelyClone();
				return style;
			}
			else
			{
				StylePtr style;
				if (string_style.empty())
				{
					style = std::make_shared<Style>();

					if (dataset->GetBandCount() == 1 && dataset->GetDataType() == DT_Byte)
					{
						GDALColorTable* poColorTable = dataset->GetColorTable(1);
						if (poColorTable != nullptr)
						{
							style->set_kind(StyleType::PALETTE);
							style->set_stretch(nullptr);
							int band = 1;
							style->set_band_map(&band, 1);
						}
					}
				}
				else
				{
					style = StyleSerielizer::FromJson(string_style);
					if (style == nullptr)
					{
						LOG(ERROR) << "FromJson(request_body) error";
					}
				}

				if (s_map_style_container_.size() > 10000)
				{
					LOG(INFO) << "s_map_style_container cleared";
					s_map_style_container_.clear();
				}

				style->Prepare(dataset.get());
				s_map_style_container_[md5] = style;
				return style->CompletelyClone();
			}
		}
	}

	return std::make_shared<Style>();
}