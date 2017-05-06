#pragma once

#include <sys/epoll.h>
#include <functional>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include "timer.h"

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

	explicit clever(int fd, int flags = 0, int time_to_die = NO_TIMER);
	clever(const clever& another) noexcept;
	clever(clever&& another);
	~clever();

	private:

	void set_missing_flags(int now, int prev);

	public:

	int operator*();

	void set_flags(int flags);
	int get_flags();

	void set_timeout(int millis);
	void set_on_read(std::function <int(clever&)> func);
	void set_on_write(std::function <int(clever&)> func);

	protected:
	
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

	static const int MAX_EVENTS = 2048;

	virtual int read(epoll_event event);
	virtual int write(epoll_event event);

	void delete_all_objects(int fd, int flags);

	friend clever_socket;
	
	int fd;
	int flags;
	int time_to_die;
	//std::list <clever> responsible;
	static const clever epoll_fd;
	static timer clever_time;
	static std::map <int, std::unique_ptr <clever>> ref_descriptors;
	std::function <int(clever&)> on_read;
	std::function <int(clever&)> on_write;
};
