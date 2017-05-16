#include "simple_file_descriptor.h"
#include "log.h"
#include "fd_exception.h"
#include <fcntl.h>
#include <unistd.h>

simple_file_descriptor::simple_file_descriptor(int fd):
	valid(true),
	fd(fd)
{}

simple_file_descriptor::simple_file_descriptor(simple_file_descriptor&& another):
	valid(true),
	fd(another.fd)
{
	another.valid = false;
}

simple_file_descriptor::~simple_file_descriptor()
{
	if (is_valid())
	{
		if (fcntl(fd, F_GETFD) == -1)
		{
			throw fd_exception("Non valid file descriptor ", fd);
		}
		if (close(fd) != 0)
		{
			throw fd_exception("Error closing file descriptor ", fd);
		}
	}
}

int simple_file_descriptor::operator*() const
{
	return fd;
}

bool simple_file_descriptor::is_valid()
{
	return valid;
}

simple_file_descriptor::pointer::pointer(const pointer& another):
	fd(another.fd)
{}

simple_file_descriptor::pointer::pointer(pointer&& another):
	fd(another.fd)
{}

simple_file_descriptor::pointer::pointer(int fd):
	fd(fd)
{}

int simple_file_descriptor::pointer::operator *() const
{
	return fd;
}

simple_file_descriptor::pointer simple_file_descriptor::get_pointer() const
{
	return pointer(fd);
}

simple_file_descriptor::pointer& simple_file_descriptor::pointer::operator=(const pointer& another)
{
	fd = another.fd;
	return *this;
}

bool operator<(const simple_file_descriptor& a, const simple_file_descriptor& b)
{
	return *a < *b;
}

bool operator<(const simple_file_descriptor::pointer& a, const simple_file_descriptor::pointer& b)
{
	return *a < *b;
}
