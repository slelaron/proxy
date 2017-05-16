#include "file_descriptor.h"
#include "thread_pool.h"

using namespace std;

int main()
{
	thread_pool<10> pool;

	int pipefd[2];
	int res = pipe(pipefd);
	if (res < 0)
	{
		cerr << "Pipe creating error " << strerror(errno) << endl;
	}
	else
	{
		file_descriptor <> epoller = epoll_create1(0);
		epoll_event event;
		event.data.fd = pipefd[0];
		event.events = EPOLLIN | EPOLLET;
		epoll_ctl(*epoller, EPOLL_CTL_ADD, pipefd[0], &event);
		function <string()> func = []() -> string { return "Hello, Nikita!";};
		promise_to_task <string> promise = pool.add_task(func, file_descriptor <notifier> (pipefd[1]));
		epoll_event events[10];
		epoll_wait(*epoller, events, 10, -1);
		cerr << promise.get() << endl;
	}
	return 0;
}
