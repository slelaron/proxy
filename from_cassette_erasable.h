#include <memory>
#include "cassette.h"

struct from_cassette_erasable
{
	void set_cassete(std::shared_ptr <cassette> cas);

	protected:
	std::shared_ptr <cassette> cas;
};

struct from_cassette_erasable_client: from_cassette_erasable
{
	~from_cassette_erasable_client();
};

struct from_cassette_erasable_server: from_cassette_erasable
{
	~from_cassette_erasable_server();
};
