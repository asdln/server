#ifndef PIEIMAGESERVER_HISTOGRAM_H_
#define PIEIMAGESERVER_HISTOGRAM_H_

#include <memory>
class Dataset;

class Histogram
{
public:

	Histogram();
	~Histogram();

	void SetStats(double dMin, double dMax, double dMean, double dStdDev);

	void QueryStats(double& dMin, double& dMax, double& dMean, double& dStdDev);

	void SetHistogram(double* ppdHistogram);

	double* GetHistogram();

	void SetStep(double dStep) { step_ = dStep; };

	double GetStep() { return step_; };

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

};

typedef std::shared_ptr<Histogram> HistogramPtr;

HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic = false);

#endif //PIEIMAGESERVER_HISTOGRAM_H_