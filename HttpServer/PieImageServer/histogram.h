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
	* @brief 统计最小值
	*/
	double minimum_ = 0.0;					//统计最小值

	/**
	* @brief 统计最大值
	*/
	double maximum_ = 0.0;					//统计最大值

	/**
	* @brief 均值
	*/
	double mean_ = 0.0;				//均值

	/**
	* @brief 方差
	*/
	double std_dev_ = 0.0;				//方差

	/**
	* @brief 直方图
	*/
	double* histogram_ = nullptr;			//直方图

	/**
	* @brief 梯度
	*/
	double step_ = 1.0;				//梯度

	int class_count_ = 256;  //或者65536

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