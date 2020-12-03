#ifndef HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_
#define HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_

#include "min_max_stretch.h"

#define lut_size 65536 //256

class Dataset;

class HistogramEqualizeStretch :
	public MinMaxStretch
{
public:

	HistogramEqualizeStretch();
	~HistogramEqualizeStretch();

	void DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset) override;

	void set_stretch_percent(double stretch_percent) { stretch_percent_ = stretch_percent; }

	double stretch_percent() { return stretch_percent_; }

	virtual void Prepare(int band_count, int* band_map, Dataset* dataset) override;

	StretchPtr Clone() override;

protected:

	void Copy(HistogramEqualizeStretch*);

	void HistogramEqualize(const double* pdHistogram, int hist_size, int nBandIndex);

	template<class T>
	void DoStretchImpl_EQL(T* pData, int hist_class, unsigned char* pMaskBuffer, int nSize, int nBandCount, double* no_data_value, int* have_no_data)
	{
		//������ֱ��д��ԭ����
		unsigned char* pRenderBuffer = (unsigned char*)pData;
		for (int i = 0; i < nSize; i++)
		{
			for (int j = 0; j < nBandCount; j++)
			{
				double dCoef = (hist_class - 1.0) / (max_value_[j] - min_value_[j]);

				int pixel_index = i * nBandCount + j;
				if (pMaskBuffer[i] == 0)
				{
					pRenderBuffer[pixel_index] = 255;
					continue;
				}

				if (have_no_data[j] && no_data_value[j] == (double)(pData[pixel_index]))
				{
					pMaskBuffer[i] = 0;
					pRenderBuffer[pixel_index] = 255;
					continue;
				}

				if (pData[pixel_index] < min_value_[j])
				{
					pRenderBuffer[pixel_index] = 0;
				}
				else if (pData[pixel_index] > max_value_[j])
				{
					pRenderBuffer[pixel_index] = 255;
				}
				else
				{
					unsigned short value_temp = (pData[pixel_index] - min_value_[j]) * dCoef;
					pRenderBuffer[pixel_index] = lut_[j][value_temp];
				}
			}
		}
	}

	template<class T>
	void DoStretchImpl2_EQL(T* pData, int class_count, unsigned char* pMaskBuffer, int nSize, int nBandCount, double* no_data_value, int* have_no_data)
	{
		//������ֱ��д��ԭ����
		unsigned char* pRenderBuffer = (unsigned char*)pData;
		for (int j = 0; j < nBandCount; j++)
		{
			double dCoef = 255.0f / (max_value_[j] - min_value_[j]);
			for (int i = 0; i < nSize; i++)
			{
				int pixel_index = i * nBandCount + j;
				int pixel_index1 = pixel_index * 2;
				int pixel_index2 = pixel_index * 2 + 1;

				if (pMaskBuffer[i] == 0)
					continue;

				double dValue = sqrt(pData[pixel_index1] * pData[pixel_index1] + pData[pixel_index2] * pData[pixel_index2]);
				if (have_no_data[j] && no_data_value[j] == dValue)
				{
					pMaskBuffer[i] = 0;
					continue;
				}

				if (dValue < min_value_[j])
				{
					pRenderBuffer[pixel_index] = 0;
				}
				else if (dValue > max_value_[j])
				{
					pRenderBuffer[pixel_index] = 255;
				}
				else
				{
					pRenderBuffer[pixel_index] = (dValue - min_value_[j]) * dCoef;
				}
			}
		}
	}

protected:

	double stretch_percent_ = 0.0;

	//unsigned short lut_[4][lut_size];
	unsigned short* lut_[4] = { 0, 0, 0, 0 };
};

typedef std::shared_ptr<HistogramEqualizeStretch> HistogramEqualizeStretchPtr;

#endif  //HTTPSERVER_HISTOGRAM_EQUALIZE_STRETCH_H_