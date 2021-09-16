#include "benchmark.h"
#include <chrono>
#include "CJsonObject.hpp"
#include "etcd_storage.h"

std::atomic_uint g_qps(0); 
std::atomic_uint g_max_qps(0);
std::atomic_llong g_tile_count(0);
std::atomic_llong g_last_time(0);
std::string g_container_id;

std::shared_mutex TileTimeCount::s_mutex_;

std::map<std::thread::id, std::list< std::pair<long long, long>>> TileTimeCount::time_count_map_;

void Start_InspectThread()
{
	std::thread inspetc_thread
	([]{

		while (true)
		{
			unsigned long long tile_count = QPSLocker::GetTileCount();
			unsigned int max_qps = QPSLocker::GetMaxQPS();
			double average_time = TileTimeCount::GetAverageTileTime();
			long long last_time = TileTimeCount::GetLastTime();

			neb::CJsonObject oJson;
			oJson.Add("tile_count", (uint64)tile_count);
			oJson.Add("max_qps", (uint32)max_qps);
			oJson.Add("average_time", average_time);
			oJson.Add("last_time", (int64)last_time);

			std::string json_str = oJson.ToString();

			EtcdStorage etcd_storage;
			etcd_storage.SetValue("benchmark/" + g_container_id, json_str, true);
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
	});

	inspetc_thread.detach();
}

void GetCurrentTimeMilliseconds(long long& milliseconds)
{
	std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
	milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

void Benchmark::TimeTag(const std::string& tag)
{
	if (switch_tag == false)
		return;

	long long milliseconds;
	GetCurrentTimeMilliseconds(milliseconds);

	std::string key = std::to_string(milliseconds) + '_' + tag;

	//重名处理
	auto itr = time_tag.find(key);
	if (itr == time_tag.end())
	{
		time_tag.emplace(key);
		time_statistics.emplace_back(std::make_pair(key, milliseconds));
	}
	else
	{
		int index = 2;
		while (true)
		{
			std::string tag_emp = key + "_A" + std::to_string(index);
			if (time_tag.find(tag_emp) == time_tag.end())
			{
				time_tag.emplace(tag_emp);
				time_statistics.emplace_back(std::make_pair(tag_emp, milliseconds));
				break;
			}

			index++;
		}
	}
}

TimeDuration::TimeDuration(long long& time_count) : time_count_(time_count)
{
	GetCurrentTimeMilliseconds(time_count_);
}

TimeDuration::~TimeDuration()
{
	long long time_end = 0;
	GetCurrentTimeMilliseconds(time_end);

	time_count_ = time_end - time_count_;
}