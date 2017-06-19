#pragma once

#include "simple_file_descriptor.h"
#include <list>
#include <set>
#include <unordered_map>
#include <chrono>

#include <boost/optional.hpp>

struct clever;

struct timer
{
	struct internal
	{
		friend timer;

		internal(internal&& another) = default;
		internal(const internal& another) = default;

		void operator=(const internal& another);
		void operator=(internal&& another);
		
		private:

		internal(std::list <std::pair <simple_file_descriptor::pointer, int>>::iterator iter, int time):
			iter(iter), time(time)
		{}

		std::list <std::pair <simple_file_descriptor::pointer, int>>::iterator iter;
		int time;
	};
	
	std::pair <int, boost::optional <simple_file_descriptor::pointer>> get_time();

	internal add(simple_file_descriptor::pointer fd, int time);
	void erase(internal);
	internal update(internal);

	private:

	std::unordered_map <int, std::list <std::pair <simple_file_descriptor::pointer, int>>> descriptors;
	std::set <std::pair <int, int>> times;
	static std::chrono::high_resolution_clock::time_point begin_time;
};
