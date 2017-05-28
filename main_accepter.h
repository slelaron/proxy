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
	typedef std::list <std::pair <std::shared_ptr <cassette>, boost::optional <int>>> type_in_map;

	main_accepter(std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map, std::shared_ptr <name_resolver> resolver, simple_file_descriptor::pointer fd):
		my_cassette(std::make_shared <cassette>()),
		map(map),
		resolver(resolver),
		host(std::make_shared <std::string>())
	{
		my_cassette->set_client_socket(fd);
	}
	
	std::function <std::pair <result_type, std::vector <accept_type>>(simple_file_descriptor::pointer, epoll_event)> get_accept()
	{
		std::shared_ptr <cassette> my_cassette = this->my_cassette;
		std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map = this->map;
		std::shared_ptr <name_resolver> resolver = this->resolver;
		std::shared_ptr <std::string> host = this->host;
		return [my_cassette, map, resolver, host](simple_file_descriptor::pointer fd, epoll_event) -> std::pair <result_type, std::vector <accept_type>>
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
					if (*the_host != *host || !my_cassette->server_still_alive())
					{
						log(*the_host);
						*host = *the_host;
						auto res = resolver->resolve(*host);
						if (res.first == name_resolver::action::NEW && map->find(res.second) == map->end())
						{
							map->insert({res.second, type_in_map()});
						}
						auto& storage = map->at(res.second);
						storage.push_back({my_cassette, my_cassette->get_client_executor().get_port()});

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
		};
	}

	std::function <result_type(simple_file_descriptor::pointer, epoll_event)> get_write()
	{
		std::shared_ptr <cassette> my_cassette = this->my_cassette;
		return [my_cassette](simple_file_descriptor::pointer fd, epoll_event) -> result_type
		{
			result_type result;
			result.splice(result.begin(), my_cassette->write_client());
			return result;
		};
	}
	
	private:

	std::shared_ptr <cassette> my_cassette;
	std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map;
	std::shared_ptr <name_resolver> resolver;
	std::shared_ptr <std::string> host;
};
