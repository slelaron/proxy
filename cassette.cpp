#include "cassette.h"

cassette::result_type cassette::set_socket(simple_file_descriptor::pointer sock, boost::optional <simple_file_descriptor::pointer>& fd, int& flags)
{
	result_type result;
	if (!fd || **fd != *sock)
	{
		flags = readable_flag | writable_flag;
	}
	if (fd && **fd != *sock)
	{
		result.push_back({*fd, descriptor_action::CLOSING_SOCKET});
	}
	//log("Hello " << *sock);
	fd = boost::make_optional <simple_file_descriptor::pointer>(sock);
	//log("Buy " << **fd);
	return result;
}

cassette::result_type cassette::stop_server()
{
	result_type result;
	if (server)
	{
		int fl = get_flags(server_flags, descriptor_action::STOP_READING | descriptor_action::STOP_WRITING);
		if (fl)
		{
			result.push_back({*server, fl});
		}
	}
	return result;
}

cassette::result_type cassette::start_server()
{
	result_type result;
	if (server)
	{
		int fl = get_flags(server_flags, descriptor_action::START_READING | descriptor_action::START_WRITING);
		if (fl)
		{
			result.push_back({*server, fl});
		}
	}
	return result;
}

cassette::result_type cassette::set_client_socket(simple_file_descriptor::pointer cl)
{
	log("Setting client");
	return set_socket(cl, client, client_flags);
}

cassette::result_type cassette::set_server_socket(simple_file_descriptor::pointer sr)
{
	log("Setting server");
	return set_socket(sr, server, server_flags);
}

cassette::result_type cassette::read(boost::optional <simple_file_descriptor::pointer>& from, boost::optional <simple_file_descriptor::pointer>& to, int& from_flags, int& to_flags, std::string from_name, std::string to_name, io_executor& from_io, io_executor& to_io)
{
	int from_ret = 0, to_ret = 0;

	result_type obj;

	log(to_name << ' ' << ((to) ? **to : -1) << " flags: " << to_flags << " | " << from_name << ' ' << ((from) ? **from : -1) << " flags: " << from_flags);
	if (from)
	{
		log(from_name << " read");
		int res = from_io.read(**from);
		if (res & descriptor_action::CLOSING_SOCKET)
		{
			from_ret |= descriptor_action::CLOSING_SOCKET;
			to_ret |= descriptor_action::CLOSING_SOCKET;
		}
		log("Delivery size " << from_io.delivery.size());
		from_ret |= (from_io.delivery.size() < from_io.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		to_ret |= (from_io.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		if (to)
		{
			to_ret = get_flags(to_flags, to_ret);
			if (to_ret)
			{
				obj.push_back({*to, to_ret});
				log(to_name << ' ' << **to << ' ' << ((to_ret & START_READING) ? "START_READING " : "") << ((to_ret & STOP_READING) ? "STOP_READING " : "") << ((to_ret & START_WRITING) ? "START_WRITING " : "") << ((to_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((to_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (to_ret & descriptor_action::CLOSING_SOCKET)
				{
					to.reset();
				}
			}
		}
		if (from)
		{
			from_ret = get_flags(from_flags, from_ret);
			if (from_ret)
			{
				obj.push_back({*from, from_ret});
				log(from_name << ' ' << **from << ' ' << ((from_ret & START_READING) ? "START_READING " : "") << ((from_ret & STOP_READING) ? "STOP_READING " : "") << ((from_ret & START_WRITING) ? "START_WRITING " : "") << ((from_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((from_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (from_ret & descriptor_action::CLOSING_SOCKET)
				{
					from.reset();
				}
			}
		}
	}
	//Think about following part of code!
	//Don't you find it strange?
	else
	{
		int res = get_flags(from_flags, descriptor_action::STOP_READING);
		obj.push_back({*from, res});
		log(from_name << ' ' << **from << ' ' << ((from_ret & START_READING) ? "START_READING " : "") << ((from_ret & STOP_READING) ? "STOP_READING " : "") << ((from_ret & START_WRITING) ? "START_WRITING " : "") << ((from_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((from_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
	}

	return obj;
}

cassette::result_type cassette::read_client()
{
	return read(client, server, client_flags, server_flags, "Client", "Server", in, out);
}

cassette::result_type cassette::read_server()
{
	return read(server, client, server_flags, client_flags, "Server", "Client", out, in);
}

cassette::result_type cassette::write(boost::optional <simple_file_descriptor::pointer>& from, boost::optional <simple_file_descriptor::pointer>& to, int& from_flags, int& to_flags, std::string from_name, std::string to_name, io_executor& from_io, io_executor& to_io)
{
	int from_ret = 0, to_ret = 0;
	
	result_type obj;

	log(to_name << ' ' << ((to) ? **to : -1) << " flags: " << to_flags << " | " << from_name << ' ' << ((from) ? **from : -1) << " flags: " << from_flags);
	if (from)
	{
		log(from_name << " write");
		to_io.write(**from);
		log("Delivery size " << to_io.delivery.size());
		to_ret |= (to_io.delivery.size() < to_io.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		from_ret |= (to_io.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		if (to)
		{
			to_ret = get_flags(to_flags, to_ret);
			if (to_ret)
			{
				obj.push_back({*to, to_ret});
				log(to_name << ' ' << **to << ' ' << ((to_ret & START_READING) ? "START_READING " : "") << ((to_ret & STOP_READING) ? "STOP_READING " : "") << ((to_ret & START_WRITING) ? "START_WRITING " : "") << ((to_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
		if (from)
		{
			from_ret = get_flags(from_flags, from_ret);
			if (from_ret)
			{
				obj.push_back({*from, from_ret});
				log(from_name << ' ' << **from << ' ' << ((from_ret & START_READING) ? "START_READING " : "") << ((from_ret & STOP_READING) ? "STOP_READING " : "") << ((from_ret & START_WRITING) ? "START_WRITING " : "") << ((from_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
	}
	//Think about following part of code!
	//Don't you find it strange?
	else
	{
		int res = get_flags(from_flags, descriptor_action::STOP_WRITING);
		obj.push_back({*from, res});
		log(from_name << ' ' << **from << ' ' << ((from_ret & START_READING) ? "START_READING " : "") << ((from_ret & STOP_READING) ? "STOP_READING " : "") << ((from_ret & START_WRITING) ? "START_WRITING " : "") << ((from_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
	}

	return obj;
}

cassette::result_type cassette::write_client()
{
	return write(client, server, client_flags, server_flags, "Client", "Server", in, out);
}

cassette::result_type cassette::write_server()
{
	return write(server, client, server_flags, client_flags, "Server", "Client", out, in);
}

void cassette::invalidate_server()
{
	server.reset();
}

void cassette::invalidate_client()
{
	client.reset();
}

cassette::result_type cassette::close()
{
	result_type obj;
	if (client)
	{
		obj.push_back({*client, descriptor_action::CLOSING_SOCKET});
	}
	
	if (server)
	{
		obj.push_back({*server, descriptor_action::CLOSING_SOCKET});
	}
	return obj;
}

const http_executor& cassette::get_client_executor()
{
	return in;
}

const io_executor& cassette::get_server_executor()
{
	return out;
}

int cassette::get_flags(int& flags, int need)
{
	int result = 0;
	if ((need & descriptor_action::START_READING) && !(flags & readable_flag))
	{
		flags |= readable_flag;
		result |= descriptor_action::START_READING;
	}
	if ((need & descriptor_action::STOP_READING) && (flags & readable_flag))
	{
		flags &= ~readable_flag;
		result |= descriptor_action::STOP_READING;
	}
	if ((need & descriptor_action::START_WRITING) && !(flags & writable_flag))
	{
		flags |= writable_flag;
		result |= descriptor_action::START_WRITING;
	}
	if ((need & descriptor_action::STOP_WRITING) && (flags & writable_flag))
	{
		flags &= ~writable_flag;
		result |= descriptor_action::STOP_WRITING;
	}
	result |= (need & descriptor_action::CLOSING_SOCKET);
	return result;
}

bool cassette::can_read(simple_file_descriptor::pointer fd)
{
	if (client && *fd == **client)
	{
		return client_flags & readable_flag;
	}
	if (server && *fd == **server)
	{
		return server_flags & readable_flag;
	}
	return false;
}

bool cassette::can_write(simple_file_descriptor::pointer fd)
{
	if (client && *fd == **client)
	{
		return client_flags & writable_flag;
	}
	if (server && *fd == **server)
	{
		return server_flags & writable_flag;
	}
	return false;
}

bool cassette::need_new()
{
	return (!server || in.status());
}

bool cassette::server_still_alive()
{
	return static_cast <bool> (server);
}
