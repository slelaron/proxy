#pragma once

#include <sys/epoll.h>
#include <functional>
#include <unordered_map>
#include <map>
#include <sys/types.h>
#include <set>
#include <memory>
#include <list>
#include "timer.h"
#include <boost/optional.hpp>

enum clever_action
{
	STOP_WRITING = 1,
	STOP_READING = 2,
	CLOSING_SOCKET = 4,
	EXECUTION_ERROR = 8,
	WITHOUT_ACCIDENTS = 0
};

struct timer;
struct clever_socket;

struct clever
{
	static const int NO_TIMER = -1;

	explicit clever(int fd, int flags = GENTELMAN_SET, int time_to_die = NO_TIMER);
	clever(const clever& another) noexcept;
	clever(clever&& another) noexcept;
	~clever();

	clever& operator=(clever&&);
	clever& operator=(const clever&);

	private:

	void set_missing_flags(int now, int prev);

	public:

	int operator*() const;

	void set_flags(int flags);
	int get_flags();

	void set_timeout(int millis);
	void set_on_read(const std::function <int(clever&)>& func);
	void set_on_write(const std::function <int(clever&)>& func);

	void take_responsibility(clever&& irresponsible);

	protected:

	static inline void erase_file_descriptor(int descriptor);
	static inline epoll_event change_status_of_reading_writing(clever_action action, int flag, epoll_event event, int result, int fd);
	static inline void after_action(int result, epoll_event current, int fd);

	public:

	static void wait();

	static const int NON_CLOSE = 1;
	static const int ANOTHER_FLAG = 2;
	static const int READABLE = 4;
	static const int WRITABLE = 8;
	static const int LEVEL_EDGE_TRIGGERED = 16;
	static const int NON_BLOCK = 32;
	static const int GENTELMAN_SET = NON_BLOCK;
	
	protected:

	epoll_event set_epoll_flags(int flags);
	void after_set_read_write();

	static const int MAX_EVENTS = 2048;

	int read(epoll_event event);
	int write(epoll_event event);

	void delete_all_objects(int fd, int flags);

	friend clever_socket;
	
	int fd;
	int flags;
	int time_to_die;
	std::list <clever> responsible;
	static const clever epoll_fd;
	static timer clever_time;
	static std::map <int, clever> ref_descriptors;
	std::function <int(clever&, epoll_event)> on_read;
	std::function <int(clever&, epoll_event)> on_write;
	boost::optional <std::pair <std::list <clever>&, std::list <clever>::iterator> > parent;
	boost::optional <std::list <clever>::iterator> in_timer;
};
