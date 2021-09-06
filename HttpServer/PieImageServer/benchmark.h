#pragma once

#include <list>
#include <string>
#include <set>
#include <atomic>

struct ReadStatistic
{
	ReadStatistic(int width, int height)
	{
		read_width = width;
		read_height = height;
	}

	int read_width = 0;
	int read_height = 0;
	long long read_time_milliseconds = 0;
};

struct Benchmark
{
	Benchmark(bool tag):switch_tag(tag){}
	std::list<std::pair<std::string, long long>> time_statistics;
	std::set<std::string> time_tag;

	std::list<ReadStatistic> read_statistics;

	bool switch_tag = false;

	void TimeTag(const std::string& tag);
};

class TimeDuration
{
public:

	TimeDuration(long long& time_count);

	~TimeDuration();

protected:

	long long& time_count_;
};

extern std::atomic_int g_qps;

class QPSLocker
{
public:
	QPSLocker(std::atomic_int& qps) : qps_(qps)
	{
		qps_++;
	}

	~QPSLocker()
	{
		qps_--;
	}

protected:

	std::atomic_int& qps_;
};

void GetCurrentTimeMilliseconds(long long& milliseconds);