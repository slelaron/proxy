#pragma once

#include <iomanip>
#include <unistd.h>

template <typename... Args>
struct file_descriptor;

struct simple_file_descriptor;

struct file_descriptors
{
	template <typename T, typename... Args>
	static typename std::enable_if <contains_convertible_in_args <acceptable_tag, Args...>::value == 1, void>::type
	set_functions_to_acceptable(T& funcs, file_descriptor <Args...>& fd)
	{
		typedef typename get_first_convertible <acceptable_tag, Args...>::value::acceptable_descriptor static_first_accessable;
		typedef template_list <typename another_list <typename unpack_from_file_descriptor_to_tuple <static_first_accessable>::type>::next::next> static_type_to_accept_functions;

		static_type_to_accept_functions::template set<0>(fd, funcs());
	}

	static std::pair <simple_file_descriptor::pointer, file_descriptor <notifier>> get_pipe()
	{
		int pipefd[2];
		int res = pipe(pipefd);
		log("Created 2 pipes: " << pipefd[0] << ' ' << pipefd[1]);
		if (res < 0)
		{
			//Here is a big problem!
			//Not now. I catch this exception.
			throw fd_exception("Error creating pipe");
		}
		return std::make_pair <simple_file_descriptor::pointer, file_descriptor <notifier>> (pipefd[0], pipefd[1]);
	}

	static bool is_correct_connection(simple_file_descriptor::pointer check)
	{
		int error = 0;
		socklen_t len = sizeof(error);
		if (getsockopt(*check, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
		{
			return false;
		}
		return (error == 0);
	}
};
