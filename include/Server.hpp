#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "channel.hpp"
#include "parse_input.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <poll.h>
#include <string>
#include <map>
#include <sstream>
#include <set>

class Server {
public:
    Server(int port, const std::string &password); // Constructor
    ~Server(); // Destructor

    void start(); // Starts the server loop
    void setupSocket(); // Sets up the socket
    void acceptNewClient(); // Accepts a new client
    void handleClientMessage(int clientFd); // Handles messages from a client
    void parseCommand(int clientFd, const std::string& line); // Parses client commands
    void sendMsg(int clientFd, const std::string& msg); // Sends a message to client
    void removeClient(int clientFd); // Removes a client
    void removeFromPollfd(int clientFd); // Removes file descriptor from poll list

private:
    int _port; // Server port
    std::string _password; // Server password
    int _serverSocket; // Main server socket file descriptor
    std::vector<struct pollfd> _pollfds; // Vector of pollfd structures
    std::map<int, Client> _clients; // Map of client file descriptors to Client objects
    std::map<std::string, Channel> _channels; // Map of channel names to Channel objects
};

#endif // SERVER_HPP
