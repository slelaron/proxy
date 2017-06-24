#pragma once

#include <utility>

#include "simple_file_descriptor.h"

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
