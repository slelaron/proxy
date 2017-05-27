#include "timer.h"
#include "log.h"
#include "fd_exception.h"
#include <set>
#include <map>

timer::timer()
{}

void timer::add(simple_file_descriptor::pointer fd, int time)
{
	int current_time = time + clock();
	if (descriptors.find(time) == descriptors.end() || (descriptors.find(time) != descriptors.end() && descriptors.at(time).size() == 0))
	{
		times.insert({current_time, time});
	}
	descriptors[time].insert({*fd, current_time});
}

void timer::erase(simple_file_descriptor::pointer fd, int time)
{
	if (descriptors.find(time) != descriptors.end())
	{
		auto& container = descriptors[time];
		auto object = container.upper_bound(std::make_pair(*fd, -1));
		if (object->first == *fd)
		{
			container.erase(object);
			times.erase(std::make_pair(object->second, time));
		}
		else
		{
			throw fd_exception("May be it is bug. You try to delete element, that doesn't exist ", *fd);
		}
	}
	else
	{
		throw fd_exception("Wrong searching in times ", *fd);
	}
}

simple_file_descriptor::pointer timer::timeout()
{
	int time = times.begin()->second;
	times.erase(times.begin());
	auto& to_update = descriptors[time];
	simple_file_descriptor::pointer to_return = to_update.begin()->first;
	to_update.erase(to_update.begin());
	if (to_update.size() > 0)
	{
		times.insert({to_update.begin()->second, time});
	}
	return to_return;
}

int timer::get_time()
{
	if (times.empty())
	{
		return -1;
	}
	return times.begin()->first - clock();
}
