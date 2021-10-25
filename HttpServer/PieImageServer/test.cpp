#include "test.h"
#include "tiff_dataset.h"
#include <iostream>
#include<sstream>
#include "gdal_priv.h"
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "histogram.h"

void tiff_dataset_test()
{
	GDALAllRegister();
	TiffDataset* dataset = new TiffDataset;
	dataset->Open("D:/test/GF1C_MSS2_015331_20210202_MY8K1_01_066_L1A/GF1C_MSS2_015331_20210202_MY8K1_01_066_L1A.tiff");

	int band = 1;
	HistogramPtr histogram_res = nullptr;
	int band_count = dataset->GetBandCount();

	std::string histogram_content;
	if (dataset->ReadHistogramFile(histogram_content))
	{
		std::cout << "histogram file already exist..." << std::endl;

		Histogram_ContainerSTL container;
		std::istringstream is(histogram_content);
		boost::archive::binary_iarchive ia(is);
		ia >> container;

		histogram_res = container.histograms[band - 1];
	}
	else
	{
		std::cout << "begin compute histogram ..." << std::endl;
		Histogram_ContainerSTL container;

		for (int i = 0; i < band_count; i++)
		{
			HistogramPtr histogram = ComputerHistogram(dataset, i + 1, false/*g_complete_statistic*/, false, 0.0);
			container.histograms.push_back(histogram);

			if (band == i + 1)
			{
				histogram_res = histogram;
			}
		}

		std::ostringstream os;
		boost::archive::binary_oarchive oa(os);
		oa << container;

		histogram_content = os.str();
		dataset->SaveHistogramFile(histogram_content);

		std::cout << "end compute histogram ..." << std::endl;
	}

}

