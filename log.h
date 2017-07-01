#pragma once

#include <iostream>

#ifndef log
#define log(a) \
	std::cerr << a << std::endl; \
	std::cout << a << std::endl;
#endif
