#pragma once

#include "file_descriptor.h"
#include "cassette.h"
#include "name_resolver.h"
#include "wrappers.h"

#include <vector>
#include <list>
#include <map>
#include <memory>
#include <functional>

#include <boost/optional.hpp>

struct pipe_accepter
{
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> result_type;
	typedef file_descriptor <time_dependent_compile <45000>, non_blocking, readable, writable, closable> accept_type;
	typedef std::map <simple_file_descriptor::pointer, std::pair <std::shared_ptr <cassette>, boost::optional <int>>> type_in_map;
	
	pipe_accepter(std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map, std::shared_ptr <name_resolver> resolver):
		map(map),
		resolver(resolver)
	{}

	std::function <std::pair <result_type, std::vector <accept_type>>(simple_file_descriptor::pointer, epoll_event)> get_accept()
	{
		std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map = this->map;
		std::shared_ptr <name_resolver> resolver = this->resolver;
		return [=](simple_file_descriptor::pointer fd, epoll_event)
		{
			result_type result;
			std::vector <accept_type> accepted;

			auto func_to_apply = resolver->get_resolved(fd);

			if (func_to_apply != nullptr)
			{
				for (auto key_and_value: map->at(fd))
				{
					auto& element = key_and_value.second;
					auto next = func_to_apply->operator()(element.second);

					if (next)
					{
						element.first->set_server_socket(*next);
					}
					else
					{
						log("Something bad occured");
						continue;
					}
					
					auto read_func = [that = element.first](simple_file_descriptor::pointer, epoll_event)
					{
						return that->read_server();
					};

					auto write_func = [that = element.first](simple_file_descriptor::pointer, epoll_event)
					{
						return that->write_server();
					};

					auto close_func = [that = element.first](simple_file_descriptor::pointer)
					{
						return that->close_server();
					};

					accept_type accepted_fd(**next);
					accepted_fd.set_read(read_func);
					accepted_fd.set_write(write_func);
					accepted_fd.set_close(close_func);
					accepted.push_back(std::move(accepted_fd));
				}
			}
			
			map->erase(map->find(fd));

			return std::make_pair(result, std::move(accepted));
		};
	}

	private:

	std::shared_ptr <std::map <simple_file_descriptor::pointer, type_in_map>> map;
	std::shared_ptr <name_resolver> resolver;
};
