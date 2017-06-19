#include "timer.h"
#include "log.h"
#include "fd_exception.h"

#include <chrono>
#include <ratio>

void timer::internal::operator=(const internal& another)
{
	time = another.time;
	iter = another.iter;
}

void timer::internal::operator=(internal&& another)
{
	time = another.time;
	iter = another.iter;
}


std::chrono::high_resolution_clock::time_point timer::begin_time = std::chrono::high_resolution_clock::now();

timer::internal timer::add(simple_file_descriptor::pointer fd, int time)
{
	using namespace std::chrono;
	
	duration <double, std::milli> time_span = high_resolution_clock::now() - begin_time;
	int current_time = time + time_span.count();
	//log("Time to add:" << (int)time_span.count());
	if (descriptors.find(time) == descriptors.end() || (descriptors.find(time) != descriptors.end() && descriptors.at(time).empty()))
	{
		times.insert({current_time, time});
	}
	descriptors[time].push_back(std::make_pair(fd, current_time));
	auto iter = descriptors[time].end();
	return internal(--iter, time);
}

void timer::erase(internal to_erase)
{
	int time = to_erase.time;
	auto iter = to_erase.iter;
	if (descriptors.find(time) != descriptors.end())
	{
		auto& container = descriptors[time];
		if (iter == container.begin())
		{
			times.erase(std::make_pair(iter->second, time));
			container.erase(iter);
			if (!container.empty())
			{
				times.insert(std::make_pair(container.begin()->second, time));
			}
		}
		else
		{
			container.erase(iter);
		}
	}
	else
	{
		std::abort();
	}
}

timer::internal timer::update(internal to_update)
{
	int time = to_update.time;
	auto fd = to_update.iter->first;
	erase(to_update);
	return add(fd, time);
}

std::pair <int, boost::optional <simple_file_descriptor::pointer>> timer::get_time()
{
	using namespace std::chrono;
	
	if (times.empty())
	{
		return std::make_pair(-1, boost::none);
	}
	duration <double, std::milli> time_span = high_resolution_clock::now() - begin_time;
	//log("Time to del: " << (int)time_span.count());
	return std::make_pair(std::max(times.begin()->first - (int)time_span.count(), 0), boost::make_optional(descriptors[times.begin()->second].begin()->first));
}
