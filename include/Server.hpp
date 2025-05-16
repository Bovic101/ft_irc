#ifndef SERVER_HPP
#define SERVER_HPP
#include "Client.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>        // pour std::vector
#include <poll.h>
#include <string>
#include <map>

class Server {
public:
    Server(int port, const std::string &password);
	~Server();

    void start(); // This will start the server loop
	void setupSocket(); // This will set up the server socket
	void acceptNewClient(); // This will accept new clients
	void removeClient(int clientFd); // This will accept new clients

	// Other methods to handle client communication, etc.

private:
    int _port;
    std::string _password;
	std::vector<struct pollfd> _pollfds;
	std::map<int, Client> _clients; // Map to store clients by their file descriptor
	int _serverSocket;

};
#endif
