#include "log.h"
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
	fd = boost::make_optional <simple_file_descriptor::pointer>(sock);
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
		if (res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR))
		{
			from_ret |= res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR);
			to_ret |= res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR);
			if (to)
			{
				obj.push_back({*to, to_ret});
			}
			if (from)
			{
				obj.push_back({*from, from_ret});
			}
		}
		else
		{
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
				}
			}
			if (from)
			{
				from_ret = get_flags(from_flags, from_ret);
				if (from_ret)
				{
					obj.push_back({*from, from_ret});
					log(from_name << ' ' << **from << ' ' << ((from_ret & START_READING) ? "START_READING " : "") << ((from_ret & STOP_READING) ? "STOP_READING " : "") << ((from_ret & START_WRITING) ? "START_WRITING " : "") << ((from_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((from_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				}
			}
		}
	}
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
		int res = to_io.write(**from);
		if (res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR))
		{
			from_ret |= res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR);
			to_ret |= res & (descriptor_action::CLOSING_SOCKET | descriptor_action::EXECUTION_ERROR);
			if (to)
			{
				obj.push_back({*to, to_ret});
			}
			if (from)
			{
				obj.push_back({*from, from_ret});
			}
		}
		else
		{
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
	}
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

void cassette::upwrite_client()
{
	if (client)
	{
		if (out.delivery.size() > 0)
		{
			log("UPWRITING CLIENT");
			out.write(*client);
		}
	}
}

void cassette::upwrite_server()
{
	if (server)
	{
		if (in.delivery.size() > 0)
		{
			log("UPWRITING SERVER");
			in.write(*server);
		}
	}
}

cassette::result_type cassette::close_client()
{
	result_type obj;
	log("Buffers sizes: out " << out.delivery.size() << " in " << in.delivery.size() << ", alive: client " << static_cast <bool> (client) << " server " << static_cast <bool> (server));
	upwrite_server();
	upwrite_client();
	if (server)
	{
		obj.push_back({*server, descriptor_action::CLOSING_SOCKET});
	}
	client.reset();
	static_cast <io_executor*> (&in)->reset();
	static_cast <io_executor*> (&out)->reset();
	return obj;
}

cassette::result_type cassette::close_server()
{
	result_type obj;
	log("Buffers sizes: out " << out.delivery.size() << " in " << in.delivery.size() << ", alive: client " << static_cast <bool> (client) << " server " << static_cast <bool> (server));
	upwrite_client();
	server.reset();
	static_cast <io_executor*> (&in)->reset();
	static_cast <io_executor*> (&out)->reset();
	return obj;
}

http_executor& cassette::get_client_executor()
{
	return in;
}

io_executor& cassette::get_server_executor()
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

bool cassette::need_new()
{
	return (!server || in.status());
}

bool cassette::server_still_alive()
{
	return static_cast <bool> (server);
}

boost::optional <simple_file_descriptor::pointer> cassette::get_server()
{
	return server;
}

boost::optional <simple_file_descriptor::pointer> cassette::get_client()
{
	return client;
}
