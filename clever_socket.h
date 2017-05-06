#pragma once

#include "clever.h"
#include <functional>

struct clever_socket: clever
{
	static const int BACKLOGMAX = 100;
	static const int NECESSARY_PORT = 8001;
	
	clever_socket();

	void set_on_read(std::function <int(clever&, clever&)> func);

	protected:

	std::function <int(clever&, clever&)> on_read;

	virtual int read(epoll_event event);
};
