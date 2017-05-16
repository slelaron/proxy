#pragma once

#include "simple_file_descriptor.h"
#include <set>
#include <unordered_map>

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
	static const int TIME_TO_BREAK = 5000;
	
	timer();

	int get_time();

	// TODO: EVERYTHING HERE IS BAD! CHECK AND CORRECT!!!

	void add(simple_file_descriptor::pointer fd, int time);

	void erase(simple_file_descriptor::pointer fd, int time);

	simple_file_descriptor::pointer timeout();

	private:

	std::unordered_map <int, std::set <std::pair <int, int>>> descriptors;
	std::set <std::pair <int, int>> times;
};
