#pragma once

#include <memory>
#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include "type_def.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

class Dataset;
class Histogram;
typedef std::shared_ptr<Histogram> HistogramPtr;

class Histogram
{
	friend class boost::serialization::access;
	BOOST_SERIALIZATION_SPLIT_MEMBER()

public:

	Histogram();
	~Histogram();

	template<class Archive>
	void save(Archive& ar, const unsigned int version) const
	{
		// note, version is always the latest when saving
		ar& minimum_;
		ar& maximum_;
		ar& mean_;
		ar& std_dev_;
		ar& step_;
		ar& class_count_;

		for (int i = 0; i < class_count_; i ++)
		{
			ar& histogram_[i];
		}
	}


	template<class Archive>
	void load(Archive& ar, const unsigned int version)
	{
		ar& minimum_;
		ar& maximum_;
		ar& mean_;
		ar& std_dev_;
		ar& step_;
		ar& class_count_;

		if (histogram_)
		{
			delete histogram_;
			histogram_ = nullptr;
		}

		histogram_ = new double[class_count_];

		for (int i = 0; i < class_count_; i++)
		{
			ar& histogram_[i];
		}
	}

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

class Histogram_ContainerSTL
{
	friend class boost::serialization::access;

public:
	
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& histograms;
	}

public:

	std::vector<HistogramPtr> histograms;
};


HistogramPtr ComputerHistogram(Dataset* dataset, int band, bool complete_statistic = false, bool use_external_no_data = false, double external_no_data_value = 0.0);

void CalcPercentMinMax_SetHistogram(DataType dt, double* pdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue);

void CalcPercentMinMax(DataType dt, const double* ppdHistogram, int hist_class
	, double dParam, double dStep, double dRawMin, double& dMinValue, double& dMaxValue, int& nMin, int& nMax);

void CalFitHistogram(void* pData, DataType datatype, long lTempX, long lTempY, bool bHaveNoData, double dfNoDataValue,
	double& dStep, double& dfMin, double& dfMax, double& dfMean, double& dfStdDev, double* pdHistogram, int hist_class,
	bool bFitMin = false, bool bFitMax = false, double dFitMin = 0.0, double dFitMax = 0.0);

int CalcClassCount(DataType dt);