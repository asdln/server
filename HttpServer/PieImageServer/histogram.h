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

};

typedef std::shared_ptr<Histogram> HistogramPtr;

HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic = false);

#endif //PIEIMAGESERVER_HISTOGRAM_H_