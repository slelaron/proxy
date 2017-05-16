#include "signal_exception.h"

const char* signal_exception::what() const noexcept
{
	return "Exception caused by signals: SIGTERM of SIGINT";
}
