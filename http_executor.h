#pragma once

#include "io_executor.h"
#include "simple_file_descriptor.h"
#include <boost/optional.hpp>

struct http_executor: io_executor
{
	http_executor();
	
	int read(simple_file_descriptor::pointer);

	bool status();
	void reset();
	bool is_header_full();
	std::string get_header();

	const boost::optional <std::string>& get_host();
	const boost::optional <int>& get_port();
	const boost::optional <int>& get_length();
	
	protected:

	bool end_of_header;
	int filled;
	std::string header;
	boost::optional <std::string> host;
	boost::optional <int> port;
	boost::optional <int> length;
};
