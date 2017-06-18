#pragma once

#include <iomanip>
#include <boost/optional.hpp>
#include <functional>
#include <fcntl.h>

#include "log.h"
#include "cassette.h"

template <typename T, unsigned item = 0>
struct from_container_erasable
{
	template <typename... Args>
	from_container_erasable(Args&&... args)
	{
		log("Incorrect constructor used");
	}

	from_container_erasable(from_container_erasable&& another)
	{
		log("Correct constructor used");
		std::swap(to_erase_from, another.to_erase_from);
	}
	
	void set_container(T* container, typename T::iterator iter)
	{
		to_erase_from = boost::make_optional <std::pair <T*, typename T::iterator>> (std::make_pair(container, iter));
	}

	~from_container_erasable()
	{
		if (to_erase_from)
		{ 
			log("Erasing from storage");
			to_erase_from->first->erase(to_erase_from->second);
		}
	}
	
	boost::optional <std::pair <T*, typename T::iterator>> to_erase_from;
};

template <unsigned item = 0>
struct on_close_executable
{
	template <typename... Args>
	on_close_executable(Args&&... args):
		func([]() { log("Default function: on close executable"); })
	{}

	on_close_executable(on_close_executable&& another)
	{
		std::swap(func, another.func);
	}

	void set_close_function(std::function <void()> func)
	{
		this->func = func;
	}

	~on_close_executable()
	{
		func();
	}

	private:

	std::function <void()> func;
};

struct time_dependent
{
	const static unsigned STANDART_TIME = 5000;
	
	time_dependent(int fd):
		time(STANDART_TIME)
	{}

	time_dependent(time_dependent&& another):
		time(another.time)
	{}

	void set_time(unsigned time)
	{
		this->time = time;
	}

	unsigned get_time() const
	{
		return time;
	}

	protected:
	unsigned time;
};

template <unsigned compile_time>
struct time_dependent_compile: time_dependent
{
	time_dependent_compile(int fd):
		time_dependent(fd)
	{
		set_time(compile_time);
	}

	time_dependent_compile(time_dependent_compile&& another):
		time_dependent(std::move(another))
	{}
};

struct test_class
{
	test_class(test_class&&)
	{}

	template <typename... Args>
	test_class(Args... args)
	{}
};

struct non_blocking: virtual simple_file_descriptor
{
	template <typename... Args>
	non_blocking(Args&&... args):
		simple_file_descriptor(std::forward<Args>(args)...)
	{
		log("Setting non block " << **this);
		int flags = fcntl(**this, F_GETFL, 0);
		fcntl(**this, F_SETFL, flags | O_NONBLOCK);
	}

	non_blocking(non_blocking&& another):
		simple_file_descriptor(std::move(another))
	{}

	~non_blocking()
	{
		if (is_valid())
		{
			log("Unsetting non block " << **this);
			int flags = fcntl(**this, F_GETFL, 0);
			fcntl(**this, F_SETFL, flags & (~O_NONBLOCK));
		}
	}
};

struct executable_tag
{};

template <typename R, typename... Args>
struct executable: virtual executable_tag
{
	/*template <typename... Args1>
	executable(Args1&&... args):
		func([](Args... args) -> R {throw fd_exception("Default constructor");})
	{
		log("Bad constructor");
	}*/

	executable(int):
		func([](Args... args) -> R {throw fd_exception("Default constructor");})
	{}

	executable(executable&& another)
	{
		swap(func, another.func);
	}

	executable(const executable& another):
		func(another.func)
	{}

	void set_func(std::function <R(Args...)> func)
	{
		this->func = func;
	}

	std::function <R(Args...)> get_func() const
	{
		return func;
	}
	
	protected:
	std::function <R(Args...)> func;
};

struct writable_tag: virtual executable_tag
{};

struct writable: virtual writable_tag,
					executable <std::list <std::pair <simple_file_descriptor::pointer, int>>, simple_file_descriptor::pointer, epoll_event>
{
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> acceptable_type;
	typedef executable <acceptable_type, simple_file_descriptor::pointer, epoll_event> parent;

	writable(int fd):
		parent(fd)
	{}

	writable(writable&& another):
		parent(std::move(another))
	{}
	
	void set_write(const std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)>& func)
	{
		this->func = func;
	}

	std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> get_write() const
	{
		return func;
	}
};

struct readable_tag: virtual executable_tag
{};

struct readable: virtual readable_tag,
					executable <std::list <std::pair <simple_file_descriptor::pointer, int>>, simple_file_descriptor::pointer, epoll_event>
{
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> acceptable_type;
	typedef executable <acceptable_type, simple_file_descriptor::pointer, epoll_event> parent;

	readable(int fd):
		parent(fd)
	{}

	readable(readable&& another):
		parent(std::move(another))
	{}

	void set_read(const std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)>& func)
	{
		this->func = func;
	}

	std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> get_read() const
	{
		return func;
	}
};

struct acceptable_tag: virtual executable_tag
{};

template <typename... Args>
struct acceptable: virtual executable_tag,
					virtual acceptable_tag,
					executable <std::pair <std::list <std::pair <simple_file_descriptor::pointer, int>>, std::vector <file_descriptor <Args...>>>, simple_file_descriptor::pointer, epoll_event>
{
	typedef std::pair <std::list <std::pair <simple_file_descriptor::pointer, int>>, std::vector <file_descriptor <Args...>>> acceptable_type;
	typedef file_descriptor <Args...> acceptable_descriptor;
	typedef executable<acceptable_type, simple_file_descriptor::pointer, epoll_event> parent;

	acceptable(int fd):
		parent(fd)
	{}

	acceptable(acceptable&& another):
		parent(std::move(another))
	{}

	void set_accept(std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> func)
	{
		this->func = func;
	}

	std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> get_accept() const
	{
		return this->func;
	}
};

struct closable_tag: virtual executable_tag
{};

struct closable: virtual executable_tag,
					virtual closable_tag,
					executable <std::list <std::pair <simple_file_descriptor::pointer, int>>, simple_file_descriptor::pointer>
{
	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> acceptable_type;
	typedef executable <acceptable_type, simple_file_descriptor::pointer> parent;

	closable(int fd):
		parent(fd)
	{}

	closable(closable&& another):
		parent(std::move(another))
	{}

	void set_close(std::function <acceptable_type(simple_file_descriptor::pointer)> func)
	{
		this->func = func;
	}

	std::function <acceptable_type(simple_file_descriptor::pointer)> get_close() const
	{
		return this->func;
	}
};

struct signalizable_tag
{};

struct signalizable: virtual signalizable_tag, executable <void>
{
	typedef executable <void> parent;

	signalizable(int fd):
		parent(fd)
	{}

	signalizable(signalizable&& another):
		parent(std::move(another))
	{}

	void set_signal(std::function <void()> func)
	{
		this->func = func;
	}

	std::function <void()> get_signal() const
	{
		return this->func;
	}
};

struct notifier: virtual simple_file_descriptor
{
	notifier(int fd):
		simple_file_descriptor(fd)
	{}

	void notify()
	{
		simple_file_descriptor::pointer fd(**this);
		send_notification(fd);
	}

	static void send_notification(simple_file_descriptor::pointer fd)
	{
		char buffer[4];
		buffer[0] = *fd;
		buffer[1] = *fd >> 8;
		buffer[2] = *fd >> 16;
		buffer[3] = *fd >> 24;
		write(*fd, buffer, 4);
	}
};

struct initially_not_read
{
	template <typename... Args>
	initially_not_read(Args... args)
	{}
};

struct initially_not_write
{
	template <typename... Args>
	initially_not_write(Args... args)
	{}
};

struct initially_not_accept
{
	template <typename... Args>
	initially_not_accept(Args... args)
	{}
};

struct initially_not_signalize
{
	template <typename... Args>
	initially_not_signalize(Args... args)
	{}
};

struct auto_closable
{
	template <typename... Args>
	auto_closable(Args... args)
	{}
};
