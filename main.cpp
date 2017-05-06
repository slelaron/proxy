#include "clever_socket.h"
#include "clever.h"
#include <functional>
#include "log.h"
#include "io_executor.h"

int main()
{
	clever_socket* my_socket = new clever_socket();
	my_socket->set_on_read([](clever& that, clever& accepted) -> int {
		try
		{
			std::shared_ptr <io_executor> executor = std::make_shared <io_executor>();
			accepted.set_on_read([executor](clever& that)
			{
				log("Trying to read " << *that);
				return executor->read(that);
			});
			accepted.set_on_write([executor](clever& that)
			{
				log("Trying to write " << *that);
				return executor->write(that);
			});
			return 0;
		}
		catch (...)
		{
			return clever_action::EXECUTION_ERROR;
		}
	});
	my_socket->wait();
	
	return 0;
}
