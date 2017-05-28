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

cassette::result_type cassette::read_client()
{
	int client_ret = 0, server_ret = 0;

	result_type obj;

	log(this << " Server " << ((server) ? **server : -1) << " flags: " << server_flags << " | Client " << ((client) ? **client : -1) << " flags: " << client_flags);
	if (client)
	{
		log("Client read");
		int res = in.read(**client);
		if (res & descriptor_action::CLOSING_SOCKET)
		{
			client_ret |= descriptor_action::CLOSING_SOCKET;
			server_ret |= descriptor_action::CLOSING_SOCKET;
		}
		log("Delivery size " << in.delivery.size());
		client_ret |= (in.delivery.size() < in.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		server_ret |= (in.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		if (server)
		{
			server_ret = get_flags(server_flags, server_ret);
			if (server_ret)
			{
				obj.push_back({*server, server_ret});
				log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((server_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (server_ret & descriptor_action::CLOSING_SOCKET)
				{
					server.reset();
				}
			}
		}
		if (client)
		{
			client_ret = get_flags(client_flags, client_ret);
			if (client_ret)
			{
				obj.push_back({*client, client_ret});
				log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((client_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (client_ret & descriptor_action::CLOSING_SOCKET)
				{
					client.reset();
				}
			}
		}
	}
	else
	{
		int res = get_flags(client_flags, descriptor_action::STOP_READING);
		obj.push_back({*client, res});
		log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((client_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
	}

	return obj;
}

cassette::result_type cassette::read_server()
{
	int client_ret = 0, server_ret = 0;

	result_type obj;

	log(this << " Server " << ((server) ? **server : -1) << " flags: " << server_flags << " | Client " << ((client) ? **client : -1) << " flags: " << client_flags);
	if (server)
	{
		log("Server read");
		int res = out.read(**server);
		if (res & descriptor_action::CLOSING_SOCKET)
		{
			client_ret |= descriptor_action::CLOSING_SOCKET;
			server_ret |= descriptor_action::CLOSING_SOCKET;
		}
		log("Delivery size " << out.delivery.size());
		server_ret |= (out.delivery.size() < out.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		client_ret |= (out.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		log("Client " << client << " Server " << server);
		
		if (server)
		{
			server_ret = get_flags(server_flags, server_ret);
			if (server_ret)
			{
				obj.push_back({*server, server_ret});
				log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((server_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (server_ret & descriptor_action::CLOSING_SOCKET)
				{
					server.reset();
				}
			}
		}
		if (client)
		{
			client_ret = get_flags(client_flags, client_ret);
			if (client_ret)
			{
				obj.push_back({*client, client_ret});
				log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((client_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
				if (client_ret & descriptor_action::CLOSING_SOCKET)
				{
					client.reset();
				}
			}
		}
	}
	else
	{
		int res = get_flags(server_flags, descriptor_action::STOP_READING);
		obj.push_back({*server, res});
		log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : "") << ((server_ret & CLOSING_SOCKET) ? "CLOSING_SOCKET " : ""));
	}

	return obj;
}

cassette::result_type cassette::write_client()
{
	int client_ret = 0, server_ret = 0;
	
	result_type obj;

	log(this << " Server " << ((server) ? **server : -1) << " flags: " << server_flags << " | Client " << ((client) ? **client : -1) << " flags: " << client_flags);
	if (client)
	{
		log("Client write");
		out.write(**client);
		log("Delivery size " << out.delivery.size());
		server_ret |= (out.delivery.size() < out.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		client_ret |= (out.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		log("Client " << client << " Server " << server);
		if (server)
		{
			server_ret = get_flags(server_flags, server_ret);
			if (server_ret)
			{
				obj.push_back({*server, server_ret});
				log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
		if (client)
		{
			client_ret = get_flags(client_flags, client_ret);
			if (client_ret)
			{
				obj.push_back({*client, client_ret});
				log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
	}
	else
	{
		int res = get_flags(client_flags, descriptor_action::STOP_WRITING);
		obj.push_back({*client, res});
		log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
	}

	return obj;
}

cassette::result_type cassette::write_server()
{
	int client_ret = 0, server_ret = 0;

	result_type obj;

	log(this << " Server " << ((server) ? **server : -1) << " flags: " << server_flags << " | Client " << ((client) ? **client : -1) << " flags: " << client_flags);
	if (server)
	{
		log("Server write");
		in.write(**server);
		log("Delivery size " << in.delivery.size());
		client_ret |= (in.delivery.size() < in.BUFFER_SIZE) ? descriptor_action::START_READING : descriptor_action::STOP_READING;
		server_ret |= (in.delivery.size() > 0) ? descriptor_action::START_WRITING : descriptor_action::STOP_WRITING;

		log("Client " << client << " Server " << server);
		if (server)
		{
			server_ret = get_flags(server_flags, server_ret);
			if (server_ret)
			{
				obj.push_back({*server, server_ret});
				log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
		if (client)
		{
			client_ret = get_flags(client_flags, client_ret);
			if (client_ret)
			{
				obj.push_back({*client, client_ret});
				log("Client " << **client << ' ' << ((client_ret & START_READING) ? "START_READING " : "") << ((client_ret & STOP_READING) ? "STOP_READING " : "") << ((client_ret & START_WRITING) ? "START_WRITING " : "") << ((client_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
			}
		}
	}
	else
	{
		int res = get_flags(server_flags, descriptor_action::STOP_WRITING);
		obj.push_back({*server, res});
		log("Server " << **server << ' ' << ((server_ret & START_READING) ? "START_READING " : "") << ((server_ret & STOP_READING) ? "STOP_READING " : "") << ((server_ret & START_WRITING) ? "START_WRITING " : "") << ((server_ret & STOP_WRITING) ? "STOP_WRITING " : ""));
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
	return server;
}
