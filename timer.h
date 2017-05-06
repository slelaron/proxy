#pragma once

#include "clever.h"
#include <map>
#include <set>

struct clever;

struct timer
{
	static const int TIME_TO_BREAK = 5000;
	
	timer();

	int get_time();

	// TODO: EVERYTHING HERE IS BAD! CHECK AND CORRECT!!!

	void add(clever fd, int time);

	void erase(clever fd, int time);

	clever timeout();

	private:

	std::unordered_map <int, std::set <std::pair <int, int> > > descriptors;
	std::set <std::pair <int, int> > times;
};
