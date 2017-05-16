#include "clever.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <fcntl.h>
#include <algorithm>
#include "log.h"
#include "fd_exception.h"

clever::clever(int fd, int flags, int time_to_die):
	fd(fd),
	flags(flags),
	time_to_die(time_to_die),
	on_read([](clever&, epoll_event) { log("Default read function"); return 0;}),
	on_write([](clever&, epoll_event) { log("Default write function"); return 0;})
{
	set_missing_flags(flags, 0);
}

clever::clever(const clever& another) noexcept:
	fd(another.fd),
	flags(another.flags | NON_CLOSE),
	time_to_die(another.time_to_die),
	on_read(another.on_read),
	on_write(another.on_write)
{}

clever::clever(clever&& another) noexcept:
	fd(another.fd),
	flags(another.flags),
	time_to_die(another.time_to_die)
{
	swap(on_read, another.on_read);
	swap(on_write, another.on_write);
	another.flags |= NON_CLOSE;
}

clever::~clever()
{
	log("Deleting descriptor " << fd << ' ' << this);
	delete_all_objects(fd, flags);
	if (parent)
	{
		parent->first.erase(parent->second);
		log("Deleting from responsible descriptor executed successfully");
	}
	for (auto descriptor: responsible)
	{
		log("Deleting responsible for" << &descriptor);
		delete_all_objects(descriptor.fd, descriptor.flags);
	}
}

clever& clever::operator=(const clever& rhs)
{
	flags = rhs.flags | NON_CLOSE;
	on_read = rhs.on_read;
	on_write = rhs.on_write;
	time_to_die = rhs.time_to_die;
	fd = rhs.fd;
	//think about responsible
	return *this;
}

clever& clever::operator=(clever&& rhs)
{
	std::swap(flags, rhs.flags);
	std::swap(on_read, rhs.on_read);
	std::swap(on_write, rhs.on_write);
	std::swap(responsible, rhs.responsible);
	std::swap(parent, rhs.parent);
	std::swap(time_to_die, rhs.time_to_die);
	return *this;
}

void clever::take_responsibility(clever&& irresponsible)
{
	responsible.push_back(std::move(irresponsible));
	std::list <clever>::iterator iter = responsible.end();
	iter--;
	ref_descriptors.at(*irresponsible).parent = {responsible, iter};
	//Что произойдёт с ним при удалении? Бля, как меня это заебало! Надо всё переосмыслить.
}

epoll_event clever::set_epoll_flags(int flags)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = 0;
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
	return event;
}

void clever::set_missing_flags(int now, int prev)
{
	if ((now ^ prev) & (READABLE | WRITABLE | LEVEL_EDGE_TRIGGERED))
	{
		if (!(now & (READABLE | WRITABLE)))
		{
			ref_descriptors.erase(ref_descriptors.find(fd));
			epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		}
		else if (!(prev & (READABLE | WRITABLE)))
		{
			ref_descriptors.insert(std::make_pair(fd, *this));
			epoll_event to_set = set_epoll_flags(now);
			epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd, &to_set);
		}
		else
		{
			epoll_event to_set = set_epoll_flags(now);
			epoll_ctl(*epoll_fd, EPOLL_CTL_MOD, fd, &to_set);
		}
	}
	if ((now ^ prev) & NON_BLOCK)
	{
		if (now & NON_BLOCK)
		{
			int flags = fcntl(fd, F_GETFL, 0);
			fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		}
		else
		{
			int flags = fcntl(fd, F_GETFL, 0);
			fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
		}
	}
}

int clever::operator*() const
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

void clever::after_set_read_write()
{
	if (flags & (READABLE | WRITABLE))
	{
		ref_descriptors.erase(ref_descriptors.find(fd));
		ref_descriptors.insert(std::make_pair(fd, *this));
	}
}

void clever::set_on_read(const std::function <int(clever&)>& func)
{
	on_read = [func](clever& that, epoll_event)
	{
		return func(that);
	};
	after_set_read_write();
}

void clever::set_on_write(const std::function <int(clever&)>& func)
{
	on_write = [func](clever& that, epoll_event)
	{
		return func(that);
	};
	after_set_read_write();
}

inline epoll_event clever::change_status_of_reading_writing(clever_action action, int flag, epoll_event event, int result, int fd)
{
	if (result & action)
	{
		if (event.events & flag)
		{
			event.events &= (~flag);
			log("here");
			epoll_ctl(epoll_fd.fd, EPOLL_CTL_MOD, fd, &event);
		}
	}
	else
	{
		if (!(event.events & flag))
		{
			event.events |= flag;
			log("here2");
			epoll_ctl(epoll_fd.fd, EPOLL_CTL_MOD, fd, &event);
		}
	}
	return event;
}

inline void clever::erase_file_descriptor(int descriptor)
{
	ref_descriptors.erase(ref_descriptors.find(descriptor));
	epoll_ctl(epoll_fd.fd, EPOLL_CTL_DEL, descriptor, NULL);
}

inline void clever::after_action(int result, epoll_event current, int fd)
{ 
	log("Result flags: " << result << ' ' << ((result & clever_action::STOP_READING) ? "no read" : "read") << ' ' << ((result & clever_action::STOP_WRITING) ? "no write" : "write"));
	current = change_status_of_reading_writing(clever_action::STOP_READING, EPOLLIN, current, result, fd);
	current = change_status_of_reading_writing(clever_action::STOP_WRITING, EPOLLOUT, current, result, fd);
	log("Now " << fd << " flags " << ((current.events & EPOLLIN) ? "readable" : "non readable") << ", " << ((current.events & EPOLLOUT) ? "writable" : "non writable"));
	if (result & (clever_action::CLOSING_SOCKET | clever_action::EXECUTION_ERROR))
	{
		erase_file_descriptor(fd);
	}
}

void clever::wait()
{
	epoll_event event[MAX_EVENTS];
	for (int t = 0; ; t++)
	{
		int time_to_wait = clever_time.get_time();
		log("Step " << t << " with time " << time_to_wait);
		int cnt = epoll_wait(*epoll_fd, event, MAX_EVENTS, time_to_wait);
		if (cnt == 0)
		{
			log("Timer killed execution");
			erase_file_descriptor(*clever_time.timeout());
			//TODO: timer synchronization left
		}
		for (int i = 0; i < cnt; i++)
		{
			epoll_event current = event[i];
			log(current.events << ' ' << (bool)(current.events & EPOLLIN) << ' ' << (bool)(current.events & EPOLLOUT) << ' ' << (bool)(current.events & EPOLLET));
			if (ref_descriptors.find(current.data.fd) == ref_descriptors.end())
			{
				log("Something gone wrong" << current.data.fd);
				throw fd_exception("There is no descriptor in descriptor list", current.data.fd);
			}
			else
			{
				log("Descriptor exist in descriptor list " << current.data.fd);
			}
			clever& that = ref_descriptors.at(current.data.fd);
			if (current.events & EPOLLIN)
			{
				int result = that.read(current);
				after_action(result, event[i], that.fd);
			}
			if (current.events & EPOLLOUT)
			{
				int result = that.write(current);
				after_action(result, event[i], that.fd);
			}
		}
	}
}

int clever::read(epoll_event event)
{
	return on_read(*this, event);
}

int clever::write(epoll_event event)
{
	return on_write(*this, event);
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

void clever::set_timeout(int millis)
{
	if (time_to_die != -1)
	{
		clever_time.erase(*this, time_to_die);
	}
	time_to_die = millis;
	if (millis != -1)
	{
		clever_time.add(*this, time_to_die);
	}
}

const clever clever::epoll_fd = clever(epoll_create(1));
std::map <int, clever> clever::ref_descriptors;
timer clever::clever_time = timer();
