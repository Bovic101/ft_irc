#include <iostream> 
#include <string> 
#include <exception>
#include <cstdlib> // added

#include "Server.hpp"
#include "channel.hpp"

int main(int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	try {
		Server server(port, password);
		server.start();
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}


