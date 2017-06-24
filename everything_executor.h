#include <type_traits>
#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string.h>
#include <memory>
#include <list>
#include <set>
#include <cstdlib>

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
#include "socket_descriptor.h"
#include "file_descriptor.h"

struct everything_executor
{
	static const int readable_flag = 1;
	static const int writable_flag = 2;
	static const int acceptable_flag = 4;
	static const int timer_flag = 8;
	static const int signalizable_flag = 16;
	static const int auto_closable_flag = 32;
	static const int closable_flag = 64;

	static const int max_events = 2048;
	static const int initial_flags = 0;

	typedef std::list <std::pair <simple_file_descriptor::pointer, int>> acceptable_type;
	typedef std::function <acceptable_type(simple_file_descriptor::pointer, epoll_event)> executable_function;
	typedef std::function <acceptable_type(simple_file_descriptor::pointer)> executable_function_without_epoll_event;
	
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
	{}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <signalizable_tag, Args...>::value == 1, void>::type
	set_signal(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		signaler = boost::make_optional <std::function <void()>>(fd.get_signal());
		init_fl |= signalizable_flag;
		if (!contains_in_args <initially_not_signalize, Args...>::value)
			fl |= signalizable_flag;
	}

	template <int flag, typename T, typename E, typename... Args>
	typename std::enable_if <contains_convertible_in_args <T, Args...>::value < 1, void>::type
	set_function(std::unordered_map <int, executable_function>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{}

	template <int flag, typename T, typename E, typename... Args>
	typename std::enable_if <contains_convertible_in_args <T, Args...>::value == 1, void>::type
	set_function(std::unordered_map <int, executable_function>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		storage.insert({*fd, static_cast <const E&>(fd).get_func()});
		if (flag == readable_flag && !contains_in_args <initially_not_read, Args...>::value)
		{
			fl |= flag;
		}
		else if (flag == writable_flag && !contains_in_args <initially_not_write, Args...>::value)
		{
			fl |= flag;
		}
		init_fl |= flag;
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <acceptable_tag, Args...>::value < 1, void>::type
	set_accept(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <acceptable_tag, Args...>::value == 1, void>::type
	set_accept(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
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
	{}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <time_dependent, Args...>::value == 1, void>::type
	update_timer(int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		fl |= timer_flag;
		init_fl |= timer_flag;
		times.insert({*fd, my_timer.add(fd.get_pointer(), fd.get_time())});
	}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <closable_tag, Args...>::value < 1, void>::type
	set_close(std::unordered_map <int, executable_function_without_epoll_event>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{}

	template <typename... Args>
	typename std::enable_if <contains_convertible_in_args <closable_tag, Args...>::value == 1, void>::type
	set_close(std::unordered_map <int, executable_function_without_epoll_event>& storage, int& fl, int& init_fl, const file_descriptor <Args...>& fd)
	{
		fl |= closable_flag;
		init_fl |= closable_flag;
		storage.insert({*fd, fd.get_close()});
	}

	template <typename... Args>
	int add_file_descriptor(file_descriptor <Args...> fd)
	{
		int fl = 0;
		int init_fl = 0;

		log(*fd << " -> " << ((contains_convertible_in_args <readable_tag, Args...>::value == 1) ? "Readable " : "") << ((contains_convertible_in_args <writable_tag, Args...>::value == 1) ? "Writable " : "") << ((contains_convertible_in_args <acceptable_tag, Args...>::value == 1) ? "Acceptable " : "") << ((contains_convertible_in_args <closable_tag, Args...>::value == 1) ? "Closable " : ""));
		
		set_function <writable_flag, writable_tag, writable>(writer, fl, init_fl, fd);
		set_function <readable_flag, readable_tag, readable>(reader, fl, init_fl, fd);
		set_accept(fl, init_fl, fd);
		set_signal(fl, init_fl, fd);
		set_close(closer, fl, init_fl, fd);
		update_timer(fl, init_fl, fd);

		fl |= (contains_in_args <auto_closable, Args...>::value) ? auto_closable_flag : 0;
		init_fl |= (contains_in_args <auto_closable, Args...>::value) ? auto_closable_flag : 0;

		assert(flags.find(*fd) == flags.end());
		assert(init_flags.find(*fd) == init_flags.end());
		
		flags.insert({*fd, fl});
		init_flags.insert({*fd, init_fl});
		
		epoll_event event = set_epoll_flags(*fd, fl);

		log(CYAN << "Adding descriptor " << event.data.fd << " -> " << (((bool)(event.events & EPOLLIN)) ? "EPOLLIN " : "") << (((bool)(event.events & EPOLLOUT)) ? "EPOLLOUT " : "") << (((bool)(event.events & EPOLLRDHUP)) ? "EPOLLRDHUP " : "") << (((bool)(event.events & EPOLLHUP)) ? "EPOLLHUP " : "") << (((bool)(event.events & EPOLLERR)) ? "EPOLLERR" : "") << RESET);
		
		epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *fd, &event);
		
		descriptors.insert(std::make_pair(*fd, std::make_unique <file_descriptor <Args...>> (std::move(fd))));
		return 0;
	}

	inline void total_erasing(simple_file_descriptor::pointer fd)
	{
		log(BLUE << "Closing file descriptor " << *fd << RESET);
		if (init_flags.find(*fd) == init_flags.end())
		{
			return;
		}
		
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
		if (fl & timer_flag)
		{
			my_timer.erase(times.at(*fd));
		}
		if (fl & timer_flag)
		{
			times.erase(*fd);
		}
		if (fl & closable_flag)
		{
			closer.erase(*fd);
		}
		descriptors.erase(*fd);
		epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, *fd, NULL);
	}

	inline void change_status(int epoll_flag, int flag, int& fl, epoll_event& event, int init, int res, int stop, int start)
	{
		if (init & flag)
		{
			if (res & stop)
			{
				event.events &= ~epoll_flag;
				fl &= ~flag;
			}
			else if (res & start)
			{
				event.events |= epoll_flag;
				fl |= flag;
			}
		}
	}

	inline epoll_event change_status_of_reading_writing(simple_file_descriptor::pointer fd, int init, int& fl, int res)
	{
		epoll_event event = set_epoll_flags(*fd, fl);
		change_status(EPOLLIN, readable_flag, fl, event, init, res, STOP_READING, START_READING);
		change_status(EPOLLIN, acceptable_flag, fl, event, init, res, STOP_READING, START_READING);
		change_status(EPOLLOUT, writable_flag, fl, event, init, res, STOP_WRITING, START_WRITING);
		log(CYAN << *fd << " transformed to " << (((bool)(event.events & EPOLLIN)) ? "In" : "") << ' ' << (((bool)(event.events & EPOLLOUT)) ? "Out" : "") << RESET);
		return event;
	}

	inline void after_action(std::unordered_set <int>& deleted, acceptable_type result)
	{
		log("Result.size = " << result.size());
		for (auto iter: result)
		{	
			auto fd = iter.first;

			if (init_flags.find(*fd) == init_flags.end())
			{
				continue;
			}
			
			int init = init_flags.at(*fd);
			int& fl = flags.at(*fd);
			epoll_event event = change_status_of_reading_writing(fd, init, fl, iter.second);
			
			epoll_ctl(*epoll_fd, EPOLL_CTL_MOD, *fd, &event);

			if (iter.second & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR))
			{
				if (init & closable_flag)
				{
					result.splice(result.end(), closer.at(*fd)(fd));
				}
				total_erasing(fd);
				deleted.insert(*fd);
			}
		}
	}

	void wait()
	{
		epoll_event event[max_events];
		for (int step = 0; ; step++)
		{
			log("Step " << step);
			auto time_to_wait = my_timer.get_time();
			log(RED << "Waiting with time " << time_to_wait.first << RESET);
			int cnt = epoll_wait(*epoll_fd, event, max_events, time_to_wait.first);

			std::unordered_set <int> deleted;
			
			if (cnt == 0)
			{	
				after_action(deleted, acceptable_type({std::make_pair(*time_to_wait.second, descriptor_action::CLOSING_SOCKET)}));
				log(YELLOW << "Timer killed execution" << RESET);
			}
			
			for (int i = 0; i < cnt; i++)
			{
				epoll_event current = event[i];
				
				log(CYAN << "Descriptor " << current.data.fd << " -> " << (((bool)(current.events & EPOLLIN)) ? "EPOLLIN " : "") << (((bool)(current.events & EPOLLOUT)) ? "EPOLLOUT " : "") << (((bool)(current.events & EPOLLRDHUP)) ? "EPOLLRDHUP " : "") << (((bool)(current.events & EPOLLHUP)) ? "EPOLLHUP " : "") << (((bool)(current.events & EPOLLERR)) ? "EPOLLERR" : "") << RESET);

				if (deleted.find(current.data.fd) != deleted.end())
				{
					continue;
				}

				int fl = flags.at(current.data.fd);

				if (fl & timer_flag)
				{
					times.at(current.data.fd) = my_timer.update(times.at(current.data.fd));
				}
				
				if (current.events & (EPOLLRDHUP | EPOLLERR))
				{
					after_action(deleted, acceptable_type({std::make_pair(simple_file_descriptor::pointer(current.data.fd), descriptor_action::CLOSING_SOCKET)}));
				}
				if (deleted.find(current.data.fd) == deleted.end() && (current.events & EPOLLIN))
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
					after_action(deleted, result);
				}
				if (deleted.find(current.data.fd) == deleted.end() && (current.events & EPOLLOUT) && (fl & writable_flag))
				{
					auto result = writer.at(current.data.fd)(simple_file_descriptor::pointer(current.data.fd), current);
					after_action(deleted, result);
				}
				if (deleted.find(current.data.fd) == deleted.end() && (fl & auto_closable_flag))
				{
					log("Auto-close " << current.data.fd);
					after_action(deleted, acceptable_type({std::make_pair(simple_file_descriptor::pointer(current.data.fd), descriptor_action::CLOSING_SOCKET)}));
				}
			}
		}
	}

	private:

	epoll_event set_epoll_flags(int fd, int fl)
	{
		epoll_event event;
		event.data.fd = fd;
		event.events = initial_flags;
		if (fl & (readable_flag | acceptable_flag | signalizable_flag))
		{
			event.events |= EPOLLIN;
		}
		if (fl & writable_flag)
		{
			event.events |= EPOLLOUT;
		}
		if (fd & closable_flag)
		{
			event.events |= EPOLLRDHUP;
		}
		return event;
	}
	
	std::map <int, std::unique_ptr <simple_file_descriptor>> descriptors;
	timer my_timer;
	std::unordered_map <int, executable_function> writer;
	std::unordered_map <int, executable_function> reader;
	std::unordered_map <int, executable_function> accepter;
	std::unordered_map <int, executable_function_without_epoll_event> closer;
	std::unordered_map <int, int> flags;
	std::unordered_map <int, int> init_flags;
	std::unordered_map <int, timer::internal> times;
	const file_descriptor<> epoll_fd;
	boost::optional <std::function <void()>> signaler;
};
