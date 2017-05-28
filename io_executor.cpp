#include "io_executor.h"
#include "log.h"
#include <unistd.h>
#include <string.h>

io_executor::io_executor():
	delivery(""),
	start(0)
{}

io_executor::io_executor(io_executor&& another)
{
	std::swap(another.delivery, delivery);
	std::swap(another.start, start);
}

int io_executor::read(simple_file_descriptor::pointer fd)
{
	log("Read from " << *fd);
	//log("Read " << delivery.size() << ' ' << start);
	bool initial_emptiness = (delivery.size() == 0);
	
	char buffer[BUFFER_SIZE];
	int cnt = 0;

	if (delivery.size() - start >= BUFFER_SIZE)
	{
		//log("Read overflow " << *fd);
		return descriptor_action::STOP_READING;
	}

	size_t delivery_size = BUFFER_SIZE - (delivery.size() - start);
	
	if ((cnt = ::read(*fd, buffer, delivery_size)) > 0)
	{
		//log("I have read " << cnt);
		delivery += std::string(buffer, cnt);
	}

	//log("Total delivery size " << delivery.size() - start);

	if (cnt < 0)
	{
		log("Occured error reading from " << *fd << ": " << strerror(errno));
		return descriptor_action::EXECUTION_ERROR;
	}

	if (cnt == 0)
	{
		log("I want to close file descriptors!");
		return descriptor_action::CLOSING_SOCKET;
	}

	int flags = 0;

	if (initial_emptiness)
	{
		flags |= descriptor_action::START_WRITING;
	}

	if (delivery.size() - start >= BUFFER_SIZE)
	{
		//log("Read overflow " << *fd);
		flags |= descriptor_action::STOP_READING;
	}

	return flags;
}	

int io_executor::write(simple_file_descriptor::pointer fd)
{
	log("Write from " << *fd);
	//log("Write " << delivery.size() << ' ' << start);
	//bool initial_overflow = (delivery.size() - start >= BUFFER_SIZE);
	if (delivery.size() == 0)
	{
		//log("Nothing to write " << *fd);
		return descriptor_action::STOP_WRITING;
	}
	char buffer[BUFFER_SIZE];
	int cnt = 0;
	size_t deliver_size = BUFFER_SIZE;
	deliver_size = std::min(deliver_size, delivery.size() - start);
	do
	{
		start += cnt;
		if (start == delivery.size())
			break;
		deliver_size = BUFFER_SIZE;
		deliver_size = std::min(deliver_size, delivery.size() - start);
		delivery.copy(buffer, deliver_size, start);
		cnt = ::write(*fd, buffer, deliver_size);
	}
	while (cnt == (ssize_t)deliver_size);

	start += cnt;

	if (start > delivery.size() / 2)
	{
		delivery.erase(0, start);
		start = 0;
	}

	//log("Total delivery size " << delivery.size() - start);

	if (cnt < 0)
	{
		log("Occured error writing from " << *fd << ' ' << strerror(errno));
		return descriptor_action::EXECUTION_ERROR;
	}

	int flags = 0;

	if (delivery.size() - start < BUFFER_SIZE)
	{
		flags |= descriptor_action::START_READING;
	}

	if (delivery.size() == 0)
	{
		flags |= STOP_WRITING;
	}

	return flags;
}

std::string& io_executor::get_info()
{
	return delivery;
}

const std::string& io_executor::get_info() const
{
	return delivery;
}

void io_executor::put_info(std::string&& str)
{
	std::swap(delivery, str);
}

void io_executor::put_info(const std::string& str)
{
	delivery = str;
}
