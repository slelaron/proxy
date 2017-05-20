//DEPRECATED

#include "clever_socket.h"
#include "log.h"
#include "fd_exception.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>

clever_socket::clever_socket():
	clever(socket(AF_INET, SOCK_STREAM, 0), READABLE | LEVEL_EDGE_TRIGGERED)
{
	sockaddr_in addr;
	in_addr that;
	that.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NECESSARY_PORT);
	addr.sin_addr = that;
	bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in));
	listen(fd, BACKLOGMAX);	
}

void clever_socket::set_on_accept(const std::function <int(clever&, clever&)>& func)
{
	on_read = [func, this](clever& object, epoll_event event)
	{
		log("Accepting " << event.data.fd);
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		socklen_t len = 0;
		int result = accept(*object, (sockaddr*)&addr, &len);
		if (result == -1)
		{
			log("Error occured " << *epoll_fd);
			throw fd_exception(std::string("Error accepting file descriptor: ") + strerror(errno), *object);
		}
		else
		{
			log("No error " << result);
		}
		clever accepted = clever(result);

		//Error is here. Fucking sheet. I think, that correction these mistakes leads to create new class of static methods of the class.
		//So it is very offensively. But for the future I know that creating classes in such way leads to many problems.
		//Okay I think, that I'll make something very clever. There will be emplace method.
		
		accepted.set_flags(accepted.get_flags() | clever::READABLE | clever::WRITABLE | clever::NON_BLOCK);
		//accepted.set_timeout(timer::TIME_TO_BREAK);
		auto to_return = func(object, accepted) | clever_action::STOP_WRITING;
		take_responsibility(std::move(accepted));
		return to_return;
	};
	after_set_read_write();
}
