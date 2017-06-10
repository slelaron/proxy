#pragma once

#include "file_descriptor.h"
#include "name_resolver.h"
#include "pipe_accepter.h"

#include <list>
#include <vector>
#include <map>
#include <functional>
#include <memory>

#include <boost/optional.hpp>

struct main_accepter
{
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> result_type;
	typedef file_descriptor <non_blocking, auto_closable, acceptable <time_dependent_compile <15000>, non_blocking, readable, writable>> accept_type;
	typedef std::map <simple_file_descriptor::pointer, std::pair <std::shared_ptr <cassette>, boost::optional <int>>> type_in_map;

	main_accepter(std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map, std::shared_ptr <name_resolver> resolver, simple_file_descriptor::pointer fd):
		fd(fd),
		my_cassette(std::make_shared <cassette>()),
		map(map),
		resolver(resolver)
	{
		my_cassette->set_client_socket(fd);
	}
	
	std::function <std::pair <result_type, std::vector <accept_type>>(simple_file_descriptor::pointer, epoll_event)> get_accept()
	{
		return main_reader(fd, my_cassette, map, resolver);
	}

	std::function <result_type(simple_file_descriptor::pointer, epoll_event)> get_write()
	{
		return main_writer(my_cassette);
	}
	
	private:

	struct main_reader
	{	
		main_reader(simple_file_descriptor::pointer fd,
					std::shared_ptr <cassette> my_cassette,
					std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map,
					std::shared_ptr <name_resolver> resolver):
			fd(fd),
			my_cassette(my_cassette),
			map(map),
			resolver(resolver)
		{}

		std::pair <result_type, std::vector <accept_type>> operator()(simple_file_descriptor::pointer, epoll_event)
		{
			result_type result;
			std::vector <accept_type> accepted;
			
			result.splice(result.begin(), my_cassette->read_client());
			
			if (my_cassette->need_new())
			{
				log("Need new ");
				//result.splice(result.begin(), my_cassette->stop_server());
				//result.splice(result.begin(), my_cassette->read(fd));
				auto the_host = my_cassette->get_client_executor().get_host();
				if (the_host)
				{
					if (*the_host != host || !my_cassette->server_still_alive())
					{
						log(*the_host);
						host = *the_host;
						auto res = resolver->resolve(host);
						if (res.first == name_resolver::action::NEW && map->find(res.second) == map->end())
						{
							map->insert({res.second, type_in_map()});
						}
						map->at(res.second).insert({fd, std::make_pair(my_cassette, my_cassette->get_client_executor().get_port())});
						last = res.second;
						
						if (res.first == name_resolver::action::NEW)
						{
							accept_type accepted_fd(*res.second);
							pipe_accepter my_pipe(map, resolver);
							accepted_fd.set_accept(my_pipe.get_accept());
							accepted.push_back(std::move(accepted_fd));
						}
					}
					else
					{
						//result.splice(result.begin(), my_cassette->start_server());
					}
				}
			}
			return std::make_pair(result, std::move(accepted));
		}

		~main_reader()
		{
			if (last)
			{
				if (map->find(*last) != map->end())
				{
					map->at(*last).erase(fd);
				}
				
				last.reset();
			}
		}

		private:

		simple_file_descriptor::pointer fd;
		std::shared_ptr <cassette> my_cassette;
		std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map;
		std::shared_ptr <name_resolver> resolver;
		std::string host;
		boost::optional <simple_file_descriptor::pointer> last;
	};

	struct main_writer
	{
		main_writer(std::shared_ptr <cassette> my_cassette):
			my_cassette(my_cassette)
		{}

		result_type operator()(simple_file_descriptor::pointer, epoll_event)
		{
			result_type result;
			result.splice(result.begin(), my_cassette->write_client());
			return result;
		}

		private:

		std::shared_ptr <cassette> my_cassette;
	};

	simple_file_descriptor::pointer fd;
	std::shared_ptr <cassette> my_cassette;
	std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map;
	std::shared_ptr <name_resolver> resolver;
};
