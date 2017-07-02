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
	typedef file_descriptor <non_blocking, auto_closable, acceptable <time_dependent_compile <45000>, non_blocking, readable, writable, closable>> accept_type;
	typedef std::map <simple_file_descriptor::pointer, std::pair <std::shared_ptr <cassette>, boost::optional <int>>> type_in_map;

	main_accepter(std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map, std::shared_ptr <name_resolver> resolver, simple_file_descriptor::pointer fd):
		fd(fd),
		my_cassette(std::make_shared <cassette>()),
		map(map),
		resolver(resolver),
		last(std::make_shared <boost::optional <simple_file_descriptor::pointer>>())
	{
		my_cassette->set_client_socket(fd);
	}
	
	std::function <std::pair <result_type, std::vector <accept_type>>(simple_file_descriptor::pointer, epoll_event)> get_accept()
	{
		return main_reader(fd, my_cassette, map, resolver, last);
	}

	std::function <result_type(simple_file_descriptor::pointer, epoll_event)> get_write()
	{
		return main_writer(my_cassette);
	}

	std::function <result_type(simple_file_descriptor::pointer)> get_close()
	{
		return main_closer(my_cassette, fd, map, last);
	}
	
	private:

	struct main_reader
	{	
		main_reader(simple_file_descriptor::pointer fd,
					std::shared_ptr <cassette> my_cassette,
					std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map,
					std::shared_ptr <name_resolver> resolver,
					std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> last):
			fd(fd),
			my_cassette(my_cassette),
			map(map),
			resolver(resolver),
			last(last)
		{}

		std::pair <result_type, std::vector <accept_type>> operator()(simple_file_descriptor::pointer, epoll_event event)
		{	
			result_type result;
			std::vector <accept_type> accepted;
			
			result.splice(result.begin(), my_cassette->read_client());
			
			if (my_cassette->need_new())
			{
				log("Need new ");

				auto the_host = my_cassette->get_client_executor().get_host();
				my_cassette->get_client_executor().reset();
				if (the_host)
				{
					log("Host is " << *the_host);
					if (*the_host != host || !my_cassette->server_still_alive())
					{
						log(*the_host);
						host = *the_host;

						auto prev_server = my_cassette->get_server();
						if (prev_server)
						{
							result.push_back({*prev_server, descriptor_action::CLOSING_SOCKET});
						}
						
						try
						{
							auto res = resolver->resolve(host);
							if (res.first == name_resolver::action::NEW && map->find(res.second) == map->end())
							{
								map->insert({res.second, type_in_map()});
							}
							map->at(res.second).insert({fd, std::make_pair(my_cassette, my_cassette->get_client_executor().get_port())});
							*last = res.second;
							
							if (res.first == name_resolver::action::NEW)
							{
								accept_type accepted_fd(*res.second);
								pipe_accepter my_pipe(map, resolver);
								accepted_fd.set_accept(my_pipe.get_accept());
								accepted.push_back(std::move(accepted_fd));
							}
						}
						catch (...)
						{
							result.push_back({fd, descriptor_action::CLOSING_SOCKET});
						}
					}
				}
				else if (my_cassette->get_client_executor().get_length() >= (int)my_cassette->get_client_executor().BUFFER_SIZE || my_cassette->get_client_executor().is_header_full())
				{						
					result.push_back({fd, descriptor_action::CLOSING_SOCKET});
				}
			}
			return std::make_pair(result, std::move(accepted));
		}
		private:

		simple_file_descriptor::pointer fd;
		std::shared_ptr <cassette> my_cassette;
		std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map;
		std::shared_ptr <name_resolver> resolver;
		std::string host;
		std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> last;
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

	struct main_closer
	{
		main_closer(std::shared_ptr <cassette> my_cassette,
					simple_file_descriptor::pointer fd,
					std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map> > map,
					std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> last):
			my_cassette(my_cassette),
			fd(fd),
			map(map),
			last(last)
		{}

		result_type operator()(simple_file_descriptor::pointer)
		{
			auto to_return = my_cassette->close_client();
			if (*last)
			{
				if (map->find(**last) != map->end())
				{
					map->at(**last).erase(fd);
				}	
				last->reset();
			}
			//my_cassette->invalidate_client();
			return to_return;
		}

		private:

		std::shared_ptr <cassette> my_cassette;
		simple_file_descriptor::pointer fd;
		std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map;
		std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> last;
	};

	simple_file_descriptor::pointer fd;
	std::shared_ptr <cassette> my_cassette;
	std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map;
	std::shared_ptr <name_resolver> resolver;
	std::shared_ptr <boost::optional <simple_file_descriptor::pointer>> last;
};
