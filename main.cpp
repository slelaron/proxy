#include <functional>
#include <list>
#include <fstream>
#include <chrono>
#include <cstdio>

#include "log.h"
#include "http_executor.h"
#include "file_descriptor.h"
#include "wrappers.h"
#include "signal_exception.h"
#include "thread_pool.h"
#include "file_descriptors.h"
#include "descriptor_action.h"
#include "name_resolver.h"
#include "http_parser.h"
#include "cassette.h"
#include "main_accepter.h"

int main()
{
	//freopen("output", "w", stdout);
	socket_descriptor <non_blocking, acceptable <time_dependent_compile <10000>, non_blocking, writable, closable, acceptable <non_blocking, auto_closable, acceptable <time_dependent_compile <15000>, non_blocking, readable, writable, closable>>>> needed_fd;
	everything_executor executor;

	typedef std::map <simple_file_descriptor::pointer, std::pair <std::shared_ptr <cassette>, boost::optional <int>>> type_in_mp;
	typedef std::map <simple_file_descriptor::pointer, type_in_mp> mp_type; 
	
	std::shared_ptr <mp_type> mp = std::make_shared <mp_type>();
	std::shared_ptr <name_resolver> resolver = std::make_shared <name_resolver>();
	
	auto result = [resolver, mp](simple_file_descriptor::pointer res)
	{
		main_accepter accepter(mp, resolver, res);
		return std::make_tuple(accepter.get_write(), accepter.get_close(), accepter.get_accept());
	};
	
	needed_fd.set_functions(result);
	executor.add_file_descriptor(std::move(needed_fd));
	try
	{
		executor.wait();
	}
	catch (signal_exception ex)
	{
		log("Signal caused");
	}
	return 0;
}
