#pragma once

#include <list>
#include <string>
#include <set>
#include <atomic>
#include <map>
#include <thread>
#include <shared_mutex>

extern std::atomic_uint g_qps;
extern std::atomic_uint g_max_qps;
extern std::atomic_llong g_tile_count;
extern std::atomic_llong g_last_time;

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

void GetCurrentTimeMilliseconds(long long& milliseconds);

class TileTimeCount
{
public:

	TileTimeCount() 
	{
		GetCurrentTimeMilliseconds(time_start_);
		g_last_time.store(time_start_);
	}

	~TileTimeCount()
	{
		long long time_now = 0;
		GetCurrentTimeMilliseconds(time_now);

		long time_elapsed = time_now - time_start_;

		{
			std::shared_lock<std::shared_mutex> lock(s_mutex_);
			auto itr = time_count_map_.find(std::this_thread::get_id());
			if (itr != time_count_map_.end())
			{
				auto& time_list = itr->second;

				if (time_list.size() > max_size)
					time_list.pop_front();

				time_list.emplace_back(time_start_, time_elapsed);
				return;
			}
		}

		{
			std::unique_lock<std::shared_mutex> lock(s_mutex_);
			auto itr = time_count_map_.find(std::this_thread::get_id());
			if (itr != time_count_map_.end())
			{
				auto& time_list = itr->second;

				if (time_list.size() > max_size)
					time_list.pop_front();

				time_list.emplace_back(time_start_, time_elapsed);
				return;
			}

			std::list<std::pair<long long, long>> cur_time_list{ std::make_pair(time_start_, time_elapsed) };
 			time_count_map_.emplace(std::piecewise_construct
 				, std::forward_as_tuple(std::this_thread::get_id())
 				, std::forward_as_tuple(std::move(cur_time_list)));

		}
		
	}

	static long long GetLastTime()
	{
		return g_last_time.load(std::memory_order_consume);
	}

	static double GetAverageTileTime()
	{
		std::unique_lock<std::shared_mutex> lock(s_mutex_);

		int count = 0;
		long long total_time = 0;
		for (auto itr = time_count_map_.begin(); itr != time_count_map_.end(); itr ++)
		{
			auto& time_list = itr->second;
			for (auto itr_time_list = time_list.begin(); itr_time_list != time_list.end(); itr_time_list++)
			{
				total_time += itr_time_list->second;
				count++;
			}
		}

		return total_time / (double)count;
	}

protected:

	long long time_start_;

	static std::shared_mutex s_mutex_;

	static std::map<std::thread::id, std::list< std::pair<long long, long>>> time_count_map_;

	const int max_size = 500;
};

class QPSLocker
{
public:
	QPSLocker()
	{
		g_qps++;
		g_tile_count++;

		unsigned int qps = g_qps.load(std::memory_order_consume);
		unsigned int max_qps = g_max_qps.load(std::memory_order_consume);

		if (max_qps < qps)
		{
			g_max_qps.store(qps);
		}
	}

	~QPSLocker()
	{
		g_qps--;
	}

	static unsigned int GetMaxQPS() { return g_max_qps.load(std::memory_order_consume); }

	static unsigned long long GetTileCount() { return g_tile_count.load(std::memory_order_consume); }

protected:

};

