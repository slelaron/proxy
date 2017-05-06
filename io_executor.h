#pragma once

#include "clever.h"

struct io_executor
{
	static const size_t BUFFER_SIZE = 1024;
	
	io_executor();
	io_executor(io_executor&& another);
	
	int read(clever& socket);
	int write(clever& socket);

	private:

	std::string delivery;
	size_t start;
	int flags;
};
