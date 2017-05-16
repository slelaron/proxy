#pragma once

#include <exception>

struct signal_exception: std::exception
{
	const char* what() const noexcept override;
};
