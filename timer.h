#pragma once

#include "simple_file_descriptor.h"
#include <list>
#include <set>
#include <unordered_map>
#include <chrono>

#include <boost/optional.hpp>

struct clever;

template <typename T, typename P>
struct reverse_pair_compair
{
	bool operator()(const std::pair <T, P> fst, const std::pair <T, P> snd)
	{
		return !(fst < snd);
	}
};

struct timer
{
	struct internal
	{
		friend timer;

		internal(internal&& another):
			iter(another.iter),
			time(another.time)
		{}

		internal(const internal& another):
			iter(another.iter),
			time(another.time)
		{}

		void operator=(const internal& another)
		{
			time = another.time;
			iter = another.iter;
		}

		void operator=(internal&& another)
		{
			time = another.time;
			iter = another.iter;
		}
		
		private:

		internal(std::list <std::pair <simple_file_descriptor::pointer, int>>::iterator iter, int time):
			iter(iter), time(time)
		{}

		std::list <std::pair <simple_file_descriptor::pointer, int>>::iterator iter;
		int time;
	};
	
	static const int TIME_TO_BREAK = 5000;
	
	timer();

	std::pair <int, boost::optional <simple_file_descriptor::pointer>> get_time();

	// TODO: EVERYTHING HERE IS BAD! CHECK AND CORRECT!!!

	internal add(simple_file_descriptor::pointer fd, int time);
	void erase(internal);
	internal update(internal);

	private:

	std::unordered_map <int, std::list <std::pair <simple_file_descriptor::pointer, int>>> descriptors;
	std::set <std::pair <int, int>> times;
	static std::chrono::high_resolution_clock::time_point begin_time;
};
