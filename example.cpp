#include <bits/stdc++.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

int main()
{
	addrinfo hints;
	addrinfo* result;
	int error;

	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	error = getaddrinfo("www.kgeorgiy.info", "http", &hints, &result);
	if (error != 0)
	{
		cerr << "I am here!";
	}

	cout << result << endl;
	
	return 0;
}
