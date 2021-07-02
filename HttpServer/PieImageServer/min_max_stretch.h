#pragma once

#include "stretch.h"
#include "math.h"

class Dataset;

class MinMaxStretch :
    public Stretch
{
public:

    MinMaxStretch();

    void DoStretch(void* data, unsigned char* mask_buffer, int size, int band_count, int* band_map, Dataset* dataset) override;

    virtual void Prepare(int band_count, int* band_map, Dataset* dataset) override;

    StretchPtr Clone() override;

protected:

    template<class T>
    void DoStretchImpl(T* pData, unsigned char* pMaskBuffer, int nSize, int nBandCount, double* no_data_value, int* have_no_data)
    {
        double dCoef[4];
		dCoef[0] = 255.0f / (max_value_[0] - min_value_[0]);
		dCoef[1] = 255.0f / (max_value_[1] - min_value_[1]);
		dCoef[2] = 255.0f / (max_value_[2] - min_value_[2]);
		dCoef[3] = 255.0f / (max_value_[3] - min_value_[3]);

        //拉伸结果直接写回原缓存
        unsigned char* pRenderBuffer = (unsigned char*)pData;
        for (int i = 0; i < nSize; i++)
        {
            bool pixel_no_data = true;
            for (int j = 0; j < nBandCount; j++)
            {
                int pixel_index = i * nBandCount + j;
                if (pMaskBuffer[i] == 0)
                {
                    pRenderBuffer[pixel_index] = 255;
                    continue;
                }

                if (have_no_data[j] == 0 || no_data_value[j] != (double)(pData[pixel_index]))
                {
                    pixel_no_data = false;
                }

                //全是无效值才透明
                if (pixel_no_data == true && j == nBandCount - 1)
                {
                    pMaskBuffer[i] = 0;
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
                    pRenderBuffer[pixel_index] = (pData[pixel_index] - min_value_[j]) * dCoef[j];
                }
            }
        }
    }

    template<class T>
    void DoStretchImpl2(T* pData, unsigned char* pMaskBuffer, int nSize, int nBandCount, double* no_data_value, int* have_no_data)
    {
        //拉伸结果直接写回原缓存
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

    void Copy(MinMaxStretch*);

public:

    double min_value_[4] = {0.0, 0.0, 0.0, 0.0};
    double max_value_[4] = {0.0, 0.0, 0.0, 0.0};
};

typedef std::shared_ptr<MinMaxStretch> MinMaxStretchPtr;
