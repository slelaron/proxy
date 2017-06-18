#pragma once

#include "file_descriptor.h"
#include "io_executor.h"
#include "http_executor.h"
#include "descriptor_action.h"

#include <boost/optional.hpp>
#include <vector>
#include <list>

struct cassette
{
	static const int readable_flag = 1;
	static const int writable_flag = 2;
	
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> result_type;

	result_type set_client_socket(simple_file_descriptor::pointer cl);
	result_type set_server_socket(simple_file_descriptor::pointer sr);

	result_type read_client();
	result_type write_client();
	result_type read_server();
	result_type write_server();

	const http_executor& get_client_executor();
	const io_executor& get_server_executor();

	bool can_read(simple_file_descriptor::pointer fd);
	bool can_write(simple_file_descriptor::pointer fd);

	result_type stop_server();
	result_type start_server();

	void invalidate_client();
	void invalidate_server();

	bool need_new();

	bool server_still_alive();

	result_type close();

	private:

	int get_flags(int& flags, int need);

	result_type set_socket(simple_file_descriptor::pointer sock, boost::optional <simple_file_descriptor::pointer>& fd, int& flags);
	
	boost::optional <simple_file_descriptor::pointer> client;
	boost::optional <simple_file_descriptor::pointer> server;

	int client_flags;
	int server_flags;

	http_executor in;
	io_executor out;
	
	result_type read(boost::optional <simple_file_descriptor::pointer>&, boost::optional <simple_file_descriptor::pointer>&, int&, int&, std::string, std::string, io_executor&, io_executor&);

	result_type write(boost::optional <simple_file_descriptor::pointer>&, boost::optional <simple_file_descriptor::pointer>&, int&, int&, std::string, std::string, io_executor&, io_executor&);
};
