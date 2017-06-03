#include "from_cassette_erasable.h"

void from_cassette_erasable::set_cassete(std::shared_ptr <cassette> cas)
{
	this->cas = cas;
}

from_cassette_erasable_client::~from_cassette_erasable_client()
{
	cas->invalidate_client();
}


from_cassette_erasable_server::~from_cassette_erasable_server()
{
	cas->invalidate_server();
}
