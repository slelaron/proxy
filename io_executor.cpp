#include "io_executor.h"
#include "log.h"
#include <unistd.h>
#include <string.h>

io_executor::io_executor():
	delivery(""),
	start(0),
	flags(0)
{}

io_executor::io_executor(io_executor&& another)
{
	std::swap(another.delivery, delivery);
	std::swap(another.start, start);
	std::swap(another.flags, flags);
}

int io_executor::read(clever& socket)
{
	char buffer[BUFFER_SIZE];
	int cnt = 0;

	if (delivery.size() >= 2 * BUFFER_SIZE)
	{
		log("Read overflow " << *socket);
		return flags |= clever_action::STOP_READING;
	}
	
	if ((cnt = ::read(*socket, buffer, BUFFER_SIZE)) > 0)
	{
		delivery += std::string(buffer, 0, cnt);
	}

	if (cnt < 0)
	{
		log("Occured error reading from " << *socket << ": " << strerror(errno));
		return flags | clever_action::EXECUTION_ERROR;
	}

	if (cnt == 0)
	{
		return flags |= clever_action::CLOSING_SOCKET;
	}

	flags &= ~clever_action::STOP_WRITING;

	if (delivery.size() >= 2 * BUFFER_SIZE)
	{
		log("Read overflow " << *socket);
		return flags |= clever_action::STOP_READING;
	}

	return flags;
}	

int io_executor::write(clever& socket)
{
	if (delivery.size() == 0)
	{
		log("Nothing to write " << *socket);
		return flags |= clever_action::STOP_WRITING;
	}
	char buffer[BUFFER_SIZE];
	int cnt = 0;
	size_t deliver_size = std::min(sizeof buffer, delivery.size() - start);
	do
	{
		start += cnt;
		if (start == delivery.size())
			break;
		deliver_size = std::min(sizeof buffer, delivery.size() - start);
		log("Information " << deliver_size << ' ' << start << ' ' << delivery.size());
		delivery.copy(buffer, deliver_size, start);
	}
	while ((cnt = ::write(*socket, buffer, deliver_size)) == (ssize_t)deliver_size);

	start += cnt;

	if (start > delivery.size() / 2)
	{
		delivery.erase(0, start);
		start = 0;
	}

	if (cnt < 0)
	{
		log("Occured error writing from " << *socket);
		return flags | clever_action::EXECUTION_ERROR;
	}

	if (delivery.size() < BUFFER_SIZE)
	{
		flags &= ~clever_action::STOP_READING;
	}

	if (delivery.size() == 0)
	{
		flags |= STOP_WRITING;
	}

	return flags;
}
