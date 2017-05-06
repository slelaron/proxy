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

void clever_socket::set_on_read(std::function <int(clever&, clever&)> func)
{
	on_read = func;
}

int clever_socket::read(epoll_event event)
{
	log("Accepting " << event.data.fd);
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	socklen_t len = 0;
	int result = accept(fd, (sockaddr*)&addr, &len);
	if (result == -1)
	{
		log("Error occured " << epoll_fd.fd);
		throw fd_exception(std::string("Error accepting file descriptor: ") + strerror(errno), fd);
	}
	else
	{
		log("No error " << result);
	}
	clever* accepted = new clever(result);
	accepted->set_flags(accepted->get_flags() | clever::READABLE | clever::WRITABLE | clever::NON_BLOCK);
	return on_read(*this, *accepted) | clever_action::STOP_WRITING;
}
