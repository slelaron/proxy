#include "clever.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <fcntl.h>
#include "log.h"
#include "fd_exception.h"

clever::clever(int fd, int flags, int time_to_die):
	fd(fd),
	flags(GENTELMAN_SET),
	time_to_die(time_to_die),
	on_read([](clever&) { return 0;}),
	on_write([](clever&) { return 0;})
{
	ref_descriptors.insert(std::make_pair(fd, std::unique_ptr<clever>(this)));
	set_missing_flags(flags, 0);
}

clever::clever(const clever& another) noexcept:
	fd(another.fd),
	flags(another.flags | NON_CLOSE),
	time_to_die(another.time_to_die),
	on_read(another.on_read),
	on_write(another.on_write)
{}

clever::clever(clever&& another):
	fd(another.fd),
	flags(another.flags),
	time_to_die(another.time_to_die),
	on_read(another.on_read),
	on_write(another.on_write)
{
	another.flags |= NON_CLOSE;
	ref_descriptors.erase(ref_descriptors.find(fd));
	ref_descriptors.insert(std::make_pair(fd, std::unique_ptr<clever>(this)));
}

clever::~clever()
{
	log("Deleting descriptor " << fd << ' ' << this);
	delete_all_objects(fd, flags);
	//for (auto descriptor: responsible)
	//{
		//log("Deleting responsible for" << &descriptor);
		//delete_all_objects(descriptor.fd, descriptor.flags);
	//}
}

void clever::set_missing_flags(int now, int prev)
{
	int flags = (~now) & prev;
	if (flags & (READABLE | WRITABLE))
	{
		epoll_ctl(epoll_fd.fd, EPOLL_CTL_DEL, fd, NULL);
	}
	if (flags & NON_BLOCK)
	{
		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
	}
	flags = now & (~prev);
	if (flags & (READABLE | WRITABLE))
	{
		epoll_event event;
		event.data.fd = fd;
		if (flags & READABLE)
		{
			event.events |= EPOLLIN;
		}
		if (flags & WRITABLE)
		{
			event.events |= EPOLLOUT;
		}
		if (flags & LEVEL_EDGE_TRIGGERED)
		{
			event.events |= EPOLLET;
		}
		epoll_ctl(epoll_fd.fd, EPOLL_CTL_ADD, fd, &event);
	}
	if (flags & NON_BLOCK)
	{
		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

int clever::operator*()
{
	return fd;
}

void clever::set_flags(int flags)
{
	set_missing_flags(flags, this->flags);
	this->flags = flags;
}

int clever::get_flags()
{
	return flags;
}

void clever::set_on_read(std::function <int(clever&)> func)
{
	on_read = func;
}

void clever::set_on_write(std::function <int(clever&)> func)
{
	on_write = func;
}

inline epoll_event clever::change_status_of_reading_writing(clever_action action, int flag, epoll_event event, int result, int fd)
{
	if (result & action)
	{
		if (event.events & flag)
		{
			event.events &= (~flag);
			epoll_ctl(epoll_fd.fd, EPOLL_CTL_MOD, fd, &event);
		}
	}
	else
	{
		if (!(event.events & flag))
		{
			event.events |= flag;
			epoll_ctl(epoll_fd.fd, EPOLL_CTL_MOD, fd, &event);
		}
	}
	return event;
}

inline void clever::after_action(int result, epoll_event current, int fd)
{
	current = change_status_of_reading_writing(clever_action::STOP_READING, EPOLLIN, current, result, fd);
	current = change_status_of_reading_writing(clever_action::STOP_WRITING, EPOLLOUT, current, result, fd);
	log("Now " << fd << " flags " << ((current.events & EPOLLIN) ? "readable" : "non readable") << ", " << ((current.events & EPOLLOUT) ? "writable" : "non writable"));
	if (result & (clever_action::CLOSING_SOCKET | clever_action::EXECUTION_ERROR))
	{
		ref_descriptors.at(current.data.fd).reset();
	}
}

void clever::wait()
{
	epoll_event event[MAX_EVENTS];
	for (int t = 0; ; t++)
	{
		int time_to_wait = clever_time.get_time();
		log("Step " << t << " with time " << time_to_wait);
		int cnt = epoll_wait(epoll_fd.fd, event, MAX_EVENTS, time_to_wait);
		if (cnt == 0)
		{
			log("Timer killed execution");
			//ref_descriptors.at(*clever_time.timeout()).reset();
			//TODO: timer synchronization left
		}
		for (int i = 0; i < cnt; i++)
		{
			epoll_event current = event[i];
			if (ref_descriptors.find(current.data.fd) == ref_descriptors.end())
			{
				log("Something gone wrong" << current.data.fd);
				throw fd_exception("There is no descriptor in descriptor list", current.data.fd);
			}
			else
			{
				log("Descriptor exist in descriptor list " << current.data.fd);
			}
			clever* that = ref_descriptors.at(current.data.fd).get();
			//clever_time.erase(*that, that->time_to_die);
			//clever_time.add(*that, that->time_to_die);
			if (current.events & EPOLLIN)
			{
				int result = that->read(current);
				after_action(result, event[i], that->fd);
			}
			if (current.events & EPOLLOUT)
			{
				int result = that->write(current);
				after_action(result, event[i], that->fd);
			}
		}
	}
}

int clever::read(epoll_event event)
{
	return on_read(*this);
}

int clever::write(epoll_event event)
{
	return on_write(*this);
}

void clever::delete_all_objects(int fd, int flags)
{
	if (!(flags & NON_CLOSE))
	{
		if (fcntl(fd, F_GETFD) == -1)
		{
			throw fd_exception("Non valid file descriptor");
		}
		if (flags & (READABLE | WRITABLE))
		{
			if (epoll_ctl(epoll_fd.fd, EPOLL_CTL_DEL, fd, NULL) == -1)
			{
				if (EBADF)
				{
					throw fd_exception("Non valid file descriptor ", fd);
				}
				// another fucking sheet
			}
			log("Delete operation executed successfully with " << fd);
		}
		if (close(fd) == 0)
		{
			log("Closed file descriptor " << fd);
		}
		else
		{
			throw fd_exception("Error closing file descriptor ", fd);
		}
	}
}

const clever clever::epoll_fd = clever(epoll_create(1));
std::map <int, std::unique_ptr <clever>> clever::ref_descriptors;
timer clever::clever_time = timer();
