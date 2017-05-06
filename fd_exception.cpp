#include "fd_exception.h"
#include <sstream>


fd_exception::fd_exception(std::string&& message, int fd)
{
	std::stringstream ss;
	ss << message;
	ss << fd;
	getline(ss, this->message);
}


fd_exception::fd_exception(std::string&& message)
{
	std::swap(this->message, message);
}

const char* fd_exception::what() const noexcept
{
	return message.c_str();
}

std::string fd_exception::show_message()
{
	return message;
}
