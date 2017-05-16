#include <functional>
#include <list>
#include <fstream>
#include <chrono>
#include <cstdio>

#include "log.h"
#include "io_executor.h"
#include "file_descriptor.h"
#include "wrappers.h"
#include "signal_exception.h"
#include "thread_pool.h"
#include "file_descriptors.h"
#include "descriptor_action.h"
#include "name_resolver.h"
#include "http_parser.h"

int main()
{
	//freopen("output", "w", stdout);
	socket_descriptor <non_blocking, acceptable </*time_dependent_compile <10000>,*/ non_blocking, writable, acceptable <non_blocking, auto_closable, acceptable </*time_dependent_compile <15000>,*/ non_blocking, readable, writable>>>> needed_fd;
	everything_executor executor;

	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> special_type;

	std::shared_ptr <name_resolver> resolver = std::make_shared <name_resolver>();

	typedef std::list <std::tuple <simple_file_descriptor::pointer, std::shared_ptr <io_executor>, std::shared_ptr <io_executor>, std::shared_ptr <boost::optional <simple_file_descriptor::pointer>>>> type_in_mp;

	typedef std::map <simple_file_descriptor::pointer, type_in_mp> mp_type; 
	
	std::shared_ptr <mp_type> mp = std::make_shared <mp_type>();
	
	auto result = [resolver, mp]()
	{
		std::shared_ptr <io_executor> in = std::make_shared <io_executor>();
		std::shared_ptr <io_executor> out = std::make_shared <io_executor>();
		std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> other_side = std::make_shared <boost::optional <simple_file_descriptor::pointer>>();
		auto for_accept = [resolver, in, out, other_side, mp](simple_file_descriptor::pointer fd, epoll_event)
		{
			typedef file_descriptor <non_blocking, auto_closable, acceptable </*time_dependent_compile <15000>,*/ non_blocking, readable, writable>> now_type;
			
			int res = in->read(fd);
			special_type to_return;

			if (res & (STOP_READING | START_READING))
			{
				to_return.push_back({fd, res & (STOP_READING | START_READING)});
			}
			
			if (*other_side)
			{
				if (res & (STOP_WRITING | START_WRITING))
				{
					to_return.push_back({other_side->get(), res & (STOP_WRITING | START_WRITING)});
				}
			}
			else if (http_parser::is_full(in->get_info()))
			{
				std::string host = http_parser::get_host(in->get_info());
				log("Host " << host);
				auto res = resolver->resolve(host);
				
				if (res.first == name_resolver::action::NEW)
				{
					mp->insert({res.second, type_in_mp()});
				}
				
				auto& storage = mp->operator[](res.second);
				storage.push_back(make_tuple(fd, in, out, other_side));
				
				if (res.first == name_resolver::action::NEW)
				{					
					auto for_accept = [mp, resolver](simple_file_descriptor::pointer fd1, epoll_event)
					{
						auto list = mp->operator[](fd1);
						mp->erase(mp->find(fd1));
						std::vector <file_descriptor </*time_dependent_compile <15000>,*/ non_blocking, readable, writable>> to_return;

						auto func_to_apply = resolver->get_resolved(fd1);

						log("My host name was resolved! I am so happy!");

						if (func_to_apply)
						{
							for (auto iter: list)
							{
								auto another = std::get<0>(iter);
								auto in = std::get<1>(iter);
								auto out = std::get<2>(iter);
								auto other_side = std::get<3>(iter);
								auto last_for_read = [another, out](simple_file_descriptor::pointer fd1, epoll_event)
								{
									int result = out->read(fd1);

									log("Now flags " << ((result & STOP_WRITING) ? "STOP_WRITING " : "") << ((result & START_WRITING) ? "START_WRITING " : "") << ((result & STOP_READING) ? "STOP_READING " : "") << ((result & START_READING) ? "START_READING " : ""));

									special_type to_return;
									if (result & (STOP_WRITING | START_WRITING))
									{
										log("Now flags for " << *another << ' ' << ((result_flags & STOP_WRITING) ? "STOP_WRITING " : "") << ((result_flags & START_WRITING) ? "START_WRITING " : "") << ((result_flags & STOP_READING) ? "STOP_READING " : "") << ((result_flags & START_READING) ? "START_READING " : ""));
										to_return.push_back({another, result & (STOP_WRITING | START_WRITING)});
									}
									if (result & (STOP_READING | START_READING))
									{
										log("Now flags for " << *another << ' ' << ((result & STOP_WRITING) ? "STOP_WRITING " : "") << ((result & START_WRITING) ? "START_WRITING " : "") << ((result & STOP_READING) ? "STOP_READING " : "") << ((result & START_READING) ? "START_READING " : ""));
										to_return.push_back({fd1, result & (STOP_READING | START_READING)});
									}
									return to_return;
								};

								auto last_for_write = [another, in](simple_file_descriptor::pointer fd1, epoll_event)
								{
									int result = in->write(fd1);

									log("Now flags " << ((result & STOP_WRITING) ? "STOP_WRITING " : "") << ((result & START_WRITING) ? "START_WRITING " : "") << ((result & STOP_READING) ? "STOP_READING " : "") << ((result & START_READING) ? "START_READING " : ""));
									
									special_type to_return;
									if (result & (STOP_READING | START_READING))
									{
										to_return.push_back({another, result & (STOP_READING | START_READING)});
									}
									if (result & (STOP_WRITING | START_WRITING))
									{
										to_return.push_back({fd1, result & (STOP_WRITING | START_WRITING)});
									}
									return to_return;
								};

								auto next = func_to_apply->operator()();

								if (next)
								{
									log("Accepting proxy-server file descriptors");
									*other_side = boost::make_optional <simple_file_descriptor::pointer> (**next);
									to_return.push_back(file_descriptor <time_dependent_compile <15000>, non_blocking, readable, writable>(**next));
									(to_return.end() - 1)->set_read(last_for_read);
									(to_return.end() - 1)->set_write(last_for_write);
								}
								else
								{
									log("Bad function");
								}
							}
						}
						else
						{
							log("Bad func_to_apply");
						}
						return std::make_pair(special_type(), std::move(to_return));
					};

					now_type fd(*res.second);
					fd.set_accept(for_accept);
					std::vector <now_type> vt;
					vt.push_back(std::move(fd));
					return std::make_pair(special_type(), std::move(vt));
				}

				return std::make_pair(special_type(), std::vector <now_type>());
			}
			return std::make_pair(std::move(to_return), std::vector <now_type>());
		};

		auto for_write = [out, other_side](simple_file_descriptor::pointer fd, epoll_event) -> special_type
		{
			int res = out->write(fd);
			special_type list;
			if (*other_side)
			{
				if (res & (STOP_READING | START_READING))
				{
					list.push_back({other_side->get(), res & (STOP_READING | START_READING)});
				}
			}
			if (res & (STOP_WRITING | START_WRITING))
			{
				list.push_back({fd, res & (STOP_WRITING | START_WRITING)});
			}
			
			return list;
		};

		return std::make_tuple(for_write, for_accept);
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
