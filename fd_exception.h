#pragma once

#include <exception>
#include <string>

struct fd_exception: std::exception
{
	fd_exception(std::string&& message, int fd);
	fd_exception(std::string&& message);

	const char* what() const noexcept override;

	std::string show_message();

	private:
	
	std::string message;
};
