#pragma once

#include <iostream>
#include "simple_file_descriptor.h"
#include "descriptor_action.h"

struct cassette;

struct io_executor
{
	static const size_t BUFFER_SIZE = 1024;
	
	io_executor();
	io_executor(io_executor&& another);

	std::string& get_info();
	const std::string& get_info() const;

	void put_info(const std::string&);
	void put_info(std::string&&);
	
	int read(simple_file_descriptor::pointer);
	int write(simple_file_descriptor::pointer);

	friend cassette;

	protected:

	std::string delivery;
	size_t start;
};
