#pragma once

#include <type_traits>
#include <cassert>
#include <map>
#include <unordered_map>
#include <string.h>
#include <memory>
#include <list>
#include <set>

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

#include "io_executor.h"
#include "simple_file_descriptor.h"
#include "descriptor_action.h"
#include "my_template_functions.h"
#include "timer.h"
#include "fd_exception.h"
#include "log.h"
#include "descriptor_action.h"
#include "wrappers.h"
#include "signal_exception.h"
#include "colors.h"

template <typename... Inheritance>
struct file_descriptor: virtual simple_file_descriptor,
						Inheritance...
{
	file_descriptor(int fd):
		simple_file_descriptor(fd),
		Inheritance(fd)...
	{}

	template <typename... Args>
	file_descriptor(file_descriptor<Args...>&& another):
		simple_file_descriptor(std::move(another)),
		Inheritance(std::move(another))...
	{}
};

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
		addr.sin_family = AF_INET;
		addr.sin_port = htons(NECESSARY_PORT);
		addr.sin_addr = that;
		bind(**this, (sockaddr*)&addr, sizeof(sockaddr_in));
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
			std::vector <first_accessable> vt;
			//first_accessable accepted = result;
			vt.emplace_back(result);
			type_to_accept_functions::template set<0>(vt[0], funcs());
			return std::move(std::make_pair(std::list <std::pair <simple_file_descriptor::pointer, int>>(), std::move(vt)));
		});
	}
};

struct everything_executor
{
	static const int readable_flag = 1;
	static const int writable_flag = 2;
	static const int acceptable_flag = 4;
	static const int timer_flag = 8;
	static const int signalizable_flag = 16;
	static const int auto_closable_flag = 32;

	static const int max_events = 2048;

	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> acceptable_type;
	typedef std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> executable_function;
	
	everything_executor():
		epoll_fd(epoll_create1(0))
	{
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGTERM);
		sigaddset(&mask, SIGINT);

		//Need to make good LOG

		if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		{
			throw fd_exception("Sigprocmask error");
		}

		int result = signalfd(-1, &mask, 0);
		if (result == -1)
		{
			throw fd_exception("Error occured with signal file descriptor ", result);
		}
		file_descriptor <signalizable> sig_fd(result);
		sig_fd.set_signal([](){ throw signal_exception();});
		
		
		add_file_descriptor(std::move(sig_fd));
	}

	~everything_executor()
	{}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <signalizable_tag, Args...>::value < 1, void>::type
	set_signal(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//log("Miss signal " << *fd);
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <signalizable_tag, Args...>::value == 1, void>::type
	set_signal(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//log("Set signal " << *fd);
		signaler = boost::make_optional <std::function <void()>>(fd.get_signal());
		init_fl |= signalizable_flag;
		if (!contains_in_args <initially_not_signalize, Args...>::value)
			fl |= signalizable_flag;
	}

	template <int flag, typename T, typename E, typename... Args>
	typename std::enable_if <contains_convertible_in_args <T, Args...>::value < 1, void>::type
	set_function(std::unordered_map <int, executable_function>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//if (flag == writable_flag)
		//{
			//log(GREEN << "Miss write " << *fd << RESET);
		//}
		//else
		//{
			//log(GREEN << "Miss read " << *fd << RESET);
		//}
	}

	template <int flag, typename T, typename E, typename... Args>
	typename std::enable_if <contains_convertible_in_args <T, Args...>::value == 1, void>::type
	set_function(std::unordered_map <int, executable_function>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		storage.insert({*fd, static_cast <const E&>(fd).get_func()});
		if (flag == readable_flag && !contains_in_args <initially_not_read, Args...>::value)
		{
			//log("Set read " << *fd);
			fl |= flag;
		}
		else if (flag == writable_flag && !contains_in_args <initially_not_write, Args...>::value)
		{
			//log("Set write " << *fd);
			fl |= flag;
		}
		init_fl |= flag;
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <acceptable_tag, Args...>::value < 1, void>::type
	set_accept(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//log(GREEN << "Miss accept " << *fd << RESET);
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <acceptable_tag, Args...>::value == 1, void>::type
	set_accept(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//log("Set accept " << *fd);
		executable_function func = [this, func = fd.get_accept()](simple_file_descriptor::pointer ptr, epoll_event event)
		{
			auto vt = func(ptr, event);
			for (size_t i = 0; i < vt.second.size(); i++)
			{
				add_file_descriptor(std::move(vt.second.at(i)));
			}
			return vt.first;
		};
		accepter.insert({*fd, func});
		init_fl |= acceptable_flag;
		if (!contains_in_args <initially_not_accept, Args...>::value)
			fl |= acceptable_flag;
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <time_dependent, Args...>::value < 1, void>::type
	update_timer(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		//log(GREEN << "Miss timer " << *fd << RESET);
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <time_dependent, Args...>::value == 1, void>::type
	update_timer(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		if (fd.get_time() > 0)
		{
			//log(GREEN << "Set timer" << RESET);
			my_timer.add(fd.get_pointer(), fd.get_time());
		}
		fl |= timer_flag;
		init_fl |= timer_flag;
		times.insert({*fd, fd.get_time()});
	}

	template <typename... Args>
	int add_file_descriptor(file_descriptor <Args...> fd)
	{
		int fl = 0;
		int init_fl = 0;
		
		set_function <writable_flag, writable_tag, writable>(writer, fl, init_fl, fd);
		set_function <readable_flag, readable_tag, readable>(reader, fl, init_fl, fd);
		set_accept(fl, init_fl, fd);
		set_signal(fl, init_fl, fd);
		update_timer(fl, init_fl, fd);

		fl |= (contains_in_args <auto_closable, Args...>::value) ? auto_closable_flag : 0;
		init_fl |= (contains_in_args <auto_closable, Args...>::value) ? auto_closable_flag : 0;
		
		flags.insert({*fd, fl});
		init_flags.insert({*fd, init_fl});
		
		epoll_event event = set_epoll_flags(*fd, fl);
		epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *fd, &event);
		
		descriptors.insert(std::make_pair(*fd, std::make_unique <file_descriptor <Args...>> (std::move(fd))));
		return 0;
	}

	struct erasing_by_closing_connection
	{
		static const bool need_to_erase_from_timer = true;
	};

	struct erasing_by_timeout
	{
		static const bool need_to_erase_from_timer = false;
	};

	template <typename tag = erasing_by_closing_connection>
	inline void total_erasing(simple_file_descriptor::pointer fd)
	{
		log(BLUE << "Closing file descriptor " << *fd << RESET);
		int fl = init_flags.at(*fd);
		flags.erase(*fd);
		init_flags.erase(*fd);
		if (fl & readable_flag)
		{
			reader.erase(*fd);
		}
		if (fl & writable_flag)
		{
			writer.erase(*fd);
		}
		if (fl & acceptable_flag)
		{
			accepter.erase(*fd);
		}
		if (tag::need_to_erase_from_timer && (fl & timer_flag))
		{
			my_timer.erase(fd, times.at(*fd));
			times.erase(*fd);
		}
		descriptors.erase(*fd);
		epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, *fd, NULL);
	}

	inline epoll_event change_status_of_reading_writing(simple_file_descriptor::pointer fd, int init, int& fl, int res)
	{
		epoll_event event = set_epoll_flags(*fd, fl);
		//log(CYAN << *fd << " was -> " << (((bool)(event.events & EPOLLIN)) ? "In" : "") << ' ' << (((bool)(event.events & EPOLLOUT)) ? "Out" : "") << RESET);
		if (init & readable_flag)
		{
			if (res & STOP_READING)
			{
				event.events &= ~EPOLLIN;
				fl &= ~readable_flag;
			}
			else if (res & START_READING)
			{
				event.events |= EPOLLIN;
				fl |= readable_flag;
			}
		}

		if (init & writable_flag)
		{
			if (res & STOP_WRITING)
			{
				event.events &= ~EPOLLOUT;
				fl &= ~writable_flag;
			}
			else if (res & START_WRITING)
			{
				event.events |= EPOLLOUT;
				fl |= writable_flag;
			}
		}
		//log(CYAN << *fd << " transformed to -> " << (((bool)(event.events & EPOLLIN)) ? "In" : "") << ' ' << (((bool)(event.events & EPOLLOUT)) ? "Out" : "") << RESET);
		return event;
	}

	inline void after_action(acceptable_type& result)
	{
		for (auto iter: result)
		{
			auto fd = iter.first;
			int init = init_flags.at(*fd);
			int& fl = flags.at(*fd);
			epoll_event event = change_status_of_reading_writing(fd, init, fl, iter.second);
			
			epoll_ctl(*epoll_fd, EPOLL_CTL_MOD, *fd, &event);
			
			if (iter.second & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR))
			{
				total_erasing(fd);
			}
		}
	}

	void wait()
	{
		epoll_event event[max_events];
		for (int step = 0; ; step++)
		{
			int time_to_wait = my_timer.get_time();
			//log(RED << "Waiting with time " << time_to_wait << RESET);
			int cnt = epoll_wait(*epoll_fd, event, max_events, time_to_wait);
			//log(RED << "Step " << step << RESET);
			if (cnt == 0)
			{
				total_erasing <erasing_by_timeout>(*my_timer.timeout());
				log(YELLOW << "Timer killed execution" << RESET);
			}
			for (int i = 0; i < cnt; i++)
			{
				epoll_event current = event[i];
				//log(CYAN << "Descriptor " << current.data.fd << " -> " << (bool)(current.events & EPOLLIN) << ' ' << (bool)(current.events & EPOLLOUT) << RESET);

				if (flags.find(current.data.fd) == flags.end())
				{
					continue;
				}
				
				int fl = init_flags.at(current.data.fd);
				if (current.events & EPOLLIN)
				{
					acceptable_type result; 
					if (fl & signalizable_flag)
					{
						(*signaler)();
					}
					else if (fl & readable_flag)
					{
						result = reader.at(current.data.fd)(simple_file_descriptor::pointer(current.data.fd), current);
					}
					else if (fl & acceptable_flag)
					{
						result = accepter.at(current.data.fd)(simple_file_descriptor::pointer(current.data.fd), current);
					}
					after_action(result);
				}
				if ((current.events & EPOLLOUT) && (fl & writable_flag))
				{
					acceptable_type result = writer.at(current.data.fd)(simple_file_descriptor::pointer(current.data.fd), current);
					after_action(result);
				}
				if (init_flags.find(current.data.fd) != init_flags.end() && (init_flags.at(current.data.fd) & auto_closable_flag))
				{
					total_erasing(current.data.fd);
				}
			}
		}
	}

	private:

	epoll_event set_epoll_flags(int fd, int fl)
	{
		epoll_event event;
		event.data.fd = fd;
		event.events = 0;
		//event.events |= EPOLLET;
		if (fl & (readable_flag | acceptable_flag | signalizable_flag))
		{
			event.events |= EPOLLIN;
		}
		if (fl & writable_flag)
		{
			event.events |= EPOLLOUT;
		}
		return event;
	}
	
	std::map <int, std::unique_ptr <simple_file_descriptor>> descriptors;
	timer my_timer;
	std::unordered_map <int, executable_function> writer;
	std::unordered_map <int, executable_function> reader;
	std::unordered_map <int, executable_function> accepter;
	std::unordered_map <int, int> flags;
	std::unordered_map <int, int> init_flags;
	std::unordered_map <int, int> times;
	const file_descriptor<> epoll_fd;
	boost::optional <std::function <void()>> signaler;
};
