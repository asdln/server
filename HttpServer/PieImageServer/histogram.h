#pragma once

#include <memory>
#include "utility.h"

class Dataset;

class Histogram
{
public:

	Histogram();
	~Histogram();

	void SetStats(double dMin, double dMax, double dMean, double dStdDev);

	void QueryStats(double& dMin, double& dMax, double& dMean, double& dStdDev);

	void SetHistogram(double* ppdHistogram);

	const double* GetHistogram();

	void SetStep(double dStep) { step_ = dStep; };

	double GetStep() { return step_; };

	void SetClassCount(int count) { class_count_ = count; }

	int GetClassCount() { return class_count_; }

protected:

	/**
	* @brief ͳ����Сֵ
	*/
	double minimum_ = 0.0;					//ͳ����Сֵ

	/**
	* @brief ͳ�����ֵ
	*/
	double maximum_ = 0.0;					//ͳ�����ֵ

	/**
	* @brief ��ֵ
	*/
	double mean_ = 0.0;				//��ֵ

	/**
	* @brief ����
	*/
	double std_dev_ = 0.0;				//����

	/**
	* @brief ֱ��ͼ
	*/
	double* histogram_ = nullptr;			//ֱ��ͼ

	/**
	* @brief �ݶ�
	*/
	double step_ = 1.0;				//�ݶ�

	int class_count_ = 256;  //����65536

};

typedef std::shared_ptr<Histogram> HistogramPtr;

HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic = false, bool use_external_no_data = false, double external_no_data_value = 0.0);

void CalcPercentMinMax_SetHistogram(DataType dt, double* pdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue);

void CalcPercentMinMax(DataType dt, const double* ppdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue, int& nMin, int& nMax);

void CalFitHistogram(void* pData, DataType datatype, long lTempX, long lTempY, bool bHaveNoData, double dfNoDataValue,
	double& dStep, double& dfMin, double& dfMax, double& dfMean, double& dfStdDev, double* pdHistogram, int hist_class,
	bool bFitMin = false, bool bFitMax = false, double dFitMin = 0.0, double dFitMax = 0.0);

int CalcClassCount(DataType dt);