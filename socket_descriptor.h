#pragma once 

#include <type_traits>
#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string.h>
#include <memory>
#include <list>
#include <set>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <boost/optional.hpp>

#include "simple_file_descriptor.h"
#include "descriptor_action.h"
#include "my_template_functions.h"
#include "fd_exception.h"
#include "log.h"
#include "descriptor_action.h"
#include "wrappers.h"

template <typename... Inheritance>
struct socket_descriptor: virtual simple_file_descriptor,
			virtual std::enable_if <contains_convertible_in_args <acceptable_tag, Inheritance...>::value == 1, file_descriptor <Inheritance...>>::type
{
	static const int BACKLOGMAX = 100;
	static const int NECESSARY_PORT = 8001;

	typedef typename get_first_convertible <acceptable_tag, Inheritance...>::value::acceptable_descriptor first_accessable;
	
	socket_descriptor():
		simple_file_descriptor(socket(AF_INET, SOCK_STREAM, 0)),
		file_descriptor<Inheritance...>(fd)
	{
		sockaddr_in addr;
		in_addr that;
		that.s_addr = INADDR_ANY;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(NECESSARY_PORT);
		addr.sin_addr = that;
		int possible_error = bind(**this, (sockaddr*)&addr, sizeof(sockaddr_in));
		if (possible_error)
		{
			throw fd_exception("We can't bind now. Sorry!");
		}
		listen(**this, BACKLOGMAX);
	}

	socket_descriptor(socket_descriptor&& another):
		file_descriptor<Inheritance...>(std::move(another))
	{}

	typedef template_list <typename another_list <typename unpack_from_file_descriptor_to_tuple <first_accessable>::type>::next::next> type_to_accept_functions;

	template <typename T>
	void set_functions(T& funcs)
	{
		this->set_accept([=](simple_file_descriptor::pointer, epoll_event event)
		{
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			socklen_t len = 0;
			int result = accept(event.data.fd, (sockaddr*)&addr, &len);
			if (result == -1)
			{
				throw fd_exception(std::string("Error accepting file descriptor: ") + strerror(errno), event.data.fd);
			}
			log("Accepting new socket " << result);
			std::vector <first_accessable> vt;
			vt.emplace_back(result);
			type_to_accept_functions::template set<0>(vt[0], funcs(simple_file_descriptor::pointer(result)));
			return std::move(std::make_pair(std::list <std::pair <simple_file_descriptor::pointer, int>>(), std::move(vt)));
		});
	}
};
