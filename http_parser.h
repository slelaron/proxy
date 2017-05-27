//Not used

#pragma once

#include <map>

#include "log.h"
#include "fd_exception.h"

struct http_parser
{
	http_parser():
		initialized(false)
	{}

	http_parser(const std::string& smth):
		http_parser()
	{}
	
	static bool is_full(const std::string& need)
	{
		for (size_t i = 3; i < need.size(); i++)
		{
			if (need[i - 3] == '\r' && need[i - 2] == '\n' && need[i - 1] == '\r' && need[i] == '\n')
			{
				return true;
			}
		}
		log("Wrong host name! " << need);
		return false;
	}

	static std::string get_host(const std::string& need)
	{
		std::string result = "";
		size_t pos = 0;
		for (size_t i = 4; i < need.size(); i++)
		{
			if (need[i - 4] == 'H' && need[i - 3] == 'o' && need[i - 2] == 's' && need[i - 1] == 't' && need[i] == ':')
			{
				pos = i;
				break;
			}
		}
		for (size_t i = pos + 4; i < need.size(); i++)
		{
			result += need[i - 2];
			if (need[i - 1] == '\r' && need[i] == '\n')
			{
				break;
			}
		}
		return result;
	}

	private:

	bool initialized;
};
