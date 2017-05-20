#include "name_resolver.h"
#include "file_descriptors.h"

name_resolver::name_resolver()
{}

std::pair <name_resolver::action, simple_file_descriptor::pointer> name_resolver::resolve(std::string host)
{
	if (names.find(host) != names.end())
	{
		return std::make_pair(ALREADY_EXIST, names.at(host));
	}
	else
	{
		auto res = file_descriptors::get_pipe();
		names.emplace(std::make_pair(host, res.first));
		consumers.emplace(std::move(std::make_pair(res.first, std::move(std::make_pair(host, pool.add_task(get_task(host), std::move(res.second)))))));
		return std::make_pair(NEW, res.first);
	}
}

boost::optional <std::function <boost::optional <simple_file_descriptor::pointer>()>> name_resolver::get_resolved(simple_file_descriptor::pointer fd)
{
	if (consumers.find(fd) == consumers.end())
	{
		throw fd_exception("Consumer doesn't exist");
	}
	names.erase(consumers.at(fd).first);
	if (consumers.at(fd).second.get() == NULL)
	{
		return boost::none;
	}
	auto result = get_result(consumers.at(fd).second.get());
	return boost::make_optional(result);
}

std::function <boost::optional <simple_file_descriptor::pointer>()> name_resolver::get_result(addrinfo* result)
{
	return [result]() -> boost::optional <simple_file_descriptor::pointer>
	{
		addrinfo* res;
		
		int sfd;
		for (res = result; res != NULL; res = res->ai_next)
		{
			sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

			if (sfd == -1)
				continue;

			if (connect(sfd, res->ai_addr, res->ai_addrlen) != -1)
				break;

			close(sfd);
		}

		if (res == NULL)
		{
			return boost::none;
		}

		return boost::make_optional(simple_file_descriptor::pointer(sfd));
	};
}

std::function <addrinfo*()> name_resolver::get_task(std::string host)
{
	//log("Here host " << host);
	return [host]() -> addrinfo*
	{
		addrinfo hints;
		addrinfo* result;
		int error;

		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;

		error = getaddrinfo(host.c_str(), "http", &hints, &result);
		if (error != 0)
		{
			return NULL;
		}

		return result;
	};
}
