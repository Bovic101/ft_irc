#include "Server.hpp"

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverSocket(-1) {}

Server::~Server() {
    close(_serverSocket);
}

void Server::setupSocket() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0)
        throw std::runtime_error("Failed to create socket");

    int yes = 1;
    setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    fcntl(_serverSocket, F_SETFL, O_NONBLOCK); // Make non-blocking

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Failed to bind");

    if (listen(_serverSocket, SOMAXCONN) < 0)
        throw std::runtime_error("Failed to listen");

    pollfd pfd;
    pfd.fd = _serverSocket;
    pfd.events = POLLIN;
    _pollfds.push_back(pfd);

    std::cout << "Server is listening on port " << _port << std::endl;
}

void Server::acceptNewClient() {
    sockaddr_in clientAddr;
    socklen_t clientSize = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
    if (clientFd < 0)
        return;

    fcntl(clientFd, F_SETFL, O_NONBLOCK); // Set new client to non-blocking
	Client newClient(clientFd);
	// _clients[clientFd] = newClient; // Store the client in the map
	 _clients.emplace(clientFd, Client(clientFd)); // Store the client in the map
	// _clients.emplace(clientFd, newClient); // Store the client in the map

    pollfd clientPfd;
    clientPfd.fd = clientFd;
    clientPfd.events = POLLIN;
    _pollfds.push_back(clientPfd);

    std::cout << "New client connected! FD: " << clientFd << std::endl;
}

void Server::start() {
    setupSocket();

    while (true) {
        int activity = poll(&_pollfds[0], _pollfds.size(), -1);
        if (activity < 0) {
            std::cerr << "Poll error\n";
            break;
        }

        for (size_t i = 0; i < _pollfds.size(); ++i) {
            if (_pollfds[i].revents & POLLIN) {
                if (_pollfds[i].fd == _serverSocket) {
                    acceptNewClient();
                } else {
                    handleClientMessage(_pollfds[i].fd);
					

                }
            }
        }
    }
}

void Server::handleClientMessage(int clientFd) {
	char buffer[1024];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0) {
		std::cout << "Client disconnected or error occurred\n";
		/* close(clientFd); */ removeClient(clientFd);
		return;
	}

	buffer[bytesRead] = '\0';
	// _clients[clientFd].getBuffer() += buffer;
	auto it = _clients.find(clientFd);
	if (it == _clients.end()) {
		auto result = _clients.emplace(clientFd, Client(clientFd));
		it = result.first;
	}
	it->second.appendBuffer(buffer);

	std::string& clientBuffer = it->second.getBuffer();
	// std::string& clientBuffer = _clients[clientFd].getBuffer();
	size_t newlinePos;

	while ((newlinePos = clientBuffer.find('\n')) != std::string::npos) {
		std::string message = clientBuffer.substr(0, newlinePos);
		clientBuffer.erase(0, newlinePos + 1);

		// Process the message
		parseCommand(clientFd, message);
	}

	// Echo the message back to the client
	send(clientFd, buffer, bytesRead, 0);
}

void Server::parseCommand(int clientFd, const std::string& command) {
	std::istringstream iss(command);
	std::string cmd;
	iss >> cmd;
	std::cout << "Parsed command from client " << clientFd << ": [" << cmd << "] Full line: [" << command << "]" << std::endl;
}

void Server::removeClient(int clientFd) {
	close(clientFd);
	_clients.erase(clientFd);

	// Remove the client from the pollfd list
	for (size_t i = 0; i < _pollfds.size(); ++i) {
		if (_pollfds[i].fd == clientFd) {
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}

	std::cout << "Client disconnected! FD: " << clientFd << std::endl;
}