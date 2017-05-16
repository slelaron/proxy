#pragma once

#include "clever.h"
#include <functional>
#include <sys/types.h>

struct clever_socket: clever
{
	static const int BACKLOGMAX = 100;
	static const int NECESSARY_PORT = 8001;
	
	clever_socket();

	void set_on_accept(const std::function <int(clever&, clever&)>& func);
};
