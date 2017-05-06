#include "timer.h"
#include "clever.h"
#include "log.h"
#include "fd_exception.h"
#include <set>
#include <map>

timer::timer()
{}

void timer::add(clever fd, int time)
{
	int current_time = time + clock();
	if (descriptors.find(time) == descriptors.end())
	{
		times.insert({current_time, time});
	}
	descriptors[time].insert({*fd, current_time});
}

void timer::erase(clever fd, int time)
{
	if (descriptors.find(time) != descriptors.end())
	{
		auto& container = descriptors[time];
		auto object = container.upper_bound(std::make_pair(*fd, -1));
		if (object->first == *fd)
		{
			container.erase(object);
		}
		else
		{
			throw fd_exception("May be it is bug. You try delete element, that doesn't exist ", *fd);
		}
	}
	else
	{
		throw fd_exception("Wrong searching in times ", *fd);
	}
}

clever timer::timeout()
{
	int time = times.begin()->second;
	times.erase(times.begin());
	auto& to_update = descriptors[time];
	clever to_return = clever(to_update.begin()->first, clever::NON_CLOSE);
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
