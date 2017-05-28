#include "name_resolver.h"
#include "file_descriptors.h"
#include "file_descriptor.h"

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

std::unique_ptr <name_resolver::applier> name_resolver::get_resolved(simple_file_descriptor::pointer fd)
{
	if (consumers.find(fd) == consumers.end())
	{
		throw fd_exception("Consumer doesn't exist");
	}
	log("Giving back " << consumers.at(fd).first);
	names.erase(consumers.at(fd).first);
	if (consumers.at(fd).second.get() == nullptr)
	{
		consumers.erase(consumers.find(fd));
		return nullptr;
	}
	log("Everything is OK with host " << consumers.at(fd).first);
	auto myaddrinfo = consumers.at(fd).second.get();
	consumers.erase(consumers.find(fd));
	return std::make_unique <applier> (myaddrinfo);
}

name_resolver::applier::applier(addrinfo* result):
	valid(true),
	result(result)
{}

boost::optional <simple_file_descriptor::pointer> name_resolver::applier::operator()(boost::optional <int> port)
{
	addrinfo* res;
		
	int sfd;
	for (res = result; res != NULL; res = res->ai_next)
	{
		sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

		if (sfd == -1)
			continue;

		sockaddr_in addr;
		memset(&addr, 0, sizeof(sockaddr_in));
		addr = *reinterpret_cast <sockaddr_in*> (res->ai_addr);
		//in_addr that;
		//that.s_addr = *res->ai_addr;
		//addr.sin_family = *res->ai_family;
		//addr.sin_port = htons(socket_descriptor::NECESSARY_PORT);
		//addr.sin_addr = that;
	
		if (port)
		{
			addr.sin_port = htons(*port);
		}

		if (connect(sfd, reinterpret_cast <sockaddr*>(&addr), sizeof(addr)) != -1)
			break;

		close(sfd);
	}

	if (res == NULL)
	{
		return boost::none;
	}

	return boost::make_optional <simple_file_descriptor::pointer>(simple_file_descriptor::pointer(sfd));
}

name_resolver::applier::~applier()
{
	if (valid)
	{
		freeaddrinfo(result);
	}
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
			return nullptr;
		}

		return result;
	};
}
