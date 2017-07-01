#include "http_executor.h"
#include "fd_exception.h"
#include "log.h"
#include <sstream>
#include <string>

const static std::string host_id = "Host: ";
const static std::string length_id = "Content-Length: ";
const static std::string eoh_id = "\r\n\r\n";
const static std::string eos_id = "\r\n";

http_executor::http_executor():
	io_executor(),
	end_of_header(false),
	filled(0),
	header(""),
	is_header_start(false)
{}

int http_executor::read(simple_file_descriptor::pointer smpl)
{
	int result = io_executor::read(smpl);

	bool no = end_of_header;
	if (!end_of_header)
	{
		size_t end = delivery.find(eoh_id, start);

		if (end == std::string::npos)
		{
			header += delivery.substr(start);
		}
		else
		{
			header += delivery.substr(start, end - start + 2);
			end_of_header = true;
			size_t host_pos = header.find(host_id);
			size_t host_end = header.find(eos_id, host_pos);
			size_t length_pos = header.find(length_id);
			size_t length_end = header.find(eos_id, length_pos);
			
			if (host_pos == std::string::npos || host_end == std::string::npos ||
				(length_pos != std::string::npos && length_end == std::string::npos))
			{
				//Do something with it!
				//throw fd_exception("Wrong http-request, very strange: " + header);
				return descriptor_action::EXECUTION_ERROR;
			}

			std::string prehost = header.substr(host_pos + host_id.size(), host_end - host_pos - host_id.size());
			log("Prehost: " << prehost);
			size_t port_pos = prehost.find(":");
			
			if (port_pos != std::string::npos)
			{
				host = prehost.substr(0, port_pos);
				port = std::stoi(prehost.substr(port_pos + 1));
			}
			else
			{
				host = prehost;
			}

			//erasing host from get

			std::stringstream ss;
			ss << "http://";
			ss << prehost;

			std::string s;
			std::getline(ss, s);
			log("I formed: " << s);
			size_t pos = delivery.find(s, 0);
			if (pos != std::string::npos)
			{
				delivery.erase(pos, s.size());
			}

			filled -= header.size();

			//end of erasing
			
			if (length_pos != std::string::npos)
			{
				length = std::stoi(header.substr(length_pos + length_id.size(), length_end - length_pos - length_id.size()));
				assert(end + 4 <= delivery.size());
				filled += delivery.size() - end - 4;
			}
		}
	}
	if (length)
	{
		filled += delivery.size() - start;
	}
	if (!no && end_of_header)
	{
		log(header);
	}
	return result;
}

int http_executor::write(simple_file_descriptor::pointer smpl)
{
	return io_executor::write(smpl);
}

bool http_executor::status()
{
	if (length)
	{
		if (filled <= *length)
		{
			std::cerr << filled << ' ' << *length << std::endl;
			//std::abort();
		}
		return filled >= *length;
	}
	return end_of_header;
}

void http_executor::reset()
{
	header = "";
	end_of_header = false;
	length.reset();
	host.reset();
	port.reset();
	filled = 0;
	is_header_start = true;
}

boost::optional <std::string> http_executor::get_host() const
{
	if (end_of_header)
	{
		return host;
	}
	return boost::none;
}

boost::optional <int> http_executor::get_port() const 
{
	if (end_of_header)
	{
		return port;
	}
	return boost::none;
}

boost::optional <int> http_executor::get_length() const
{
	return length;
}

bool http_executor::is_header_full() const
{
	return end_of_header;
}

std::string http_executor::get_header() const
{
	return header;
}
