#pragma once

#include "thread_pool.h"
#include "simple_file_descriptor.h"
#include "file_descriptor.h"

#include <boost/optional.hpp>
#include <map>
#include <set>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct name_resolver
{
	struct applier
	{
		applier(addrinfo*);

		boost::optional <simple_file_descriptor::pointer> operator()(boost::optional <int> port);

		~applier();

		private:
		bool valid;
		addrinfo* result;
	};
	
	enum action
	{
		ALREADY_EXIST,
		NEW
	};
	
	static const size_t thread_number = 10;
	
	name_resolver();
	
	std::pair <name_resolver::action, simple_file_descriptor::pointer> resolve(std::string);

	std::unique_ptr <applier> get_resolved(simple_file_descriptor::pointer);

	private:

	enum thread_action
	{
		TASK_SUCCESS,
		TASK_FAILURE
	};

	std::function <addrinfo*()> get_task(std::string host);
	
	friend thread_pool<thread_number>;

	private:

	auto get_result(addrinfo* result, const boost::optional <int>&);
	
	std::map <std::string, simple_file_descriptor::pointer> names;
	std::map <simple_file_descriptor::pointer, std::pair <std::string, promise_to_task <addrinfo*>>> consumers;
	thread_pool <thread_number> pool;
};
