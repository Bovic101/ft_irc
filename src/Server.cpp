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

	if (cmd == "NICK") {
		std::string nickname;
		iss >> nickname;
		if (nickname.empty()) {
			sendMsg(clientFd, "ERROR: Nickname cannot be empty\n");
			return;
		}
		_clients[clientFd].setNickname(nickname);
		sendMsg(clientFd, "Nickname set to: " + nickname + "\r\n");
	} else if (cmd == "JOIN") {
		if (_clients[clientFd].getNickname().empty()) {
			sendMsg(clientFd, "ERROR: You must set a nickname first\n");
			return;
		}
		std::string channel;
		iss >> channel;
		if (channel.empty()) {
			sendMsg(clientFd, "ERROR: Channel cannot be empty\n");
			return;
		}
		_channels[channel].insert(clientFd);
		_clients[clientFd].addChannel(channel);
		if (_channels[channel].size() == 1) {
			_channelAdmins[channel] = clientFd;
		}
		sendMsg(clientFd, "Joined channel: " + channel + "\r\n");
	} else if (cmd == "PART") {
		std::string channel;
		iss >> channel;
		if (channel.empty()) {
			sendMsg(clientFd, "ERROR: Channel cannot be empty\n");
			return;
		}
		if (_channels.find(channel) == _channels.end() || _channels[channel].find(clientFd) == _channels[channel].end()) {
			sendMsg(clientFd, "ERROR: Not in channel " + channel + "\n");
			return;
		}
		_channels[channel].erase(clientFd);
		_clients[clientFd].partChannel(channel);
		sendMsg(clientFd, "Left channel: " + channel + "\r\n");
	} else if (cmd == "QUIT") {
		sendMsg(clientFd, "Goodbye!\n");
		removeClient(clientFd);
		return;
	} else if (cmd == "PRIVMSG") {
		std::string target, message;
		iss >> target;
		std::getline(iss, message);
		if (target.empty() || message.empty()) {
			sendMsg(clientFd, "ERROR: Target and message cannot be empty\n");
			return;
		}
		if (!message.empty() && message[0] == ' ') message.erase(0, 1); 

		if (target[0] == '#') {
			if (_channels.find(target) == _channels.end() || _channels[target].find(clientFd) == _channels[target].end()) {
				sendMsg(clientFd, "ERROR: Not in channel " + target + "\n");
				return;
			}
			for (int fd : _channels[target]) {
				if (fd != clientFd) {
					sendMsg(fd, _clients[clientFd].getNickname() + ": " + message + "\n");
				}
			}
		} else {
			int targetFd = -1;
			for (const auto& pair : _clients) {
				if (pair.second.getNickname() == target) {
					targetFd = pair.first;
					break;
				}
			}
			if (targetFd == -1) {
				sendMsg(clientFd, "ERROR: User " + target + " not found\n");
				return;
			}
			sendMsg(targetFd, _clients[clientFd].getNickname() + ": " + message + "\n");
		}
	} else if (cmd == "KICK") {
		std::string channel, target;
		iss >> channel >> target;
		if (channel.empty() || target.empty()) {
			sendMsg(clientFd, "ERROR: Channel and target cannot be empty\n");
			return;
		}
		if (_channels.find(channel) == _channels.end() || _channels[channel].find(clientFd) == _channels[channel].end()) {
			sendMsg(clientFd, "ERROR: Not in channel " + channel + "\n");
			return;
		}
		if (_channels[channel].find(target) == _channels[channel].end()) {
			sendMsg(clientFd, "ERROR: User " + target + " not found in channel " + channel + "\n");
			return;
		}
		
		int targetFd = -1;
		for (const auto& pair : _clients) {
			if (pair.second.getNickname() == target) {
				targetFd = pair.first;
				break;
			}
		}
		if (targetFd == -1 || _channels[channel].find(targetFd) == _channels[channel].end()) {
			sendMsg(clientFd, "ERROR: User " + target + " not found in channel " + channel + "\n");
			return;
		}

		_channels[channel].erase(targetFd);
		_clients[targetFd].partChannel(channel);
		sendMsg(targetFd, "You have been kicked from channel " + channel + "\n");

		for (int fd : _channels[channel]) {
			if (fd != clientFd) {
				sendMsg(fd, _clients[clientFd].getNickname() + " kicked " + target + " from channel " + channel + "\n");
			}
		}
	}
    else if (cmd == "INVITE") {
        std::string username, channel;
		iss >> username >> channel;
		if (username.empty() || channel.empty()) {
			sendMsg(clientFd, "ERROR: Username and channel cannot be empty\n");
    }
	else if (cmd == "TOPIC") {
        std::string channel;
		std::string topic;
		iss >> channel;
		std::getline(iss, topic);
		if (channel.empty() || topic.empty()) {
			sendMsg(clientFd, "ERROR: Channel and topic cannot be empty\n");
			return;
		}
		if (!topic.empty() && topic[0] == ' '){
			topic.erase(0, 1);
		}
		if (topic.empty()) {
			if (_channelTopics.find(channel) != _channelTopics.end()) {
				sendMsg(clientFd, "Current topic for " + channel + ": " + _channelTopics[channel] + "\n");
			} else 
				sendMsg(clientFd, "No topic set for channel " + channel + "\n");
		} else {
			_channelTopics[channel] = topic;
			sendMsg(clientFd, "Topic for channel " + channel + " set to: " + topic + "\n");
		}
    }

    else if (cmd == "MODE") {
        // À implémenter plus tard, gérer les modes (op, voix, etc.)
        sendMsg(clientFd, "MODE command not implemented yet\r\n");
    }
    else {
        sendMsg(clientFd, "421 " + cmd + " :Unknown command\r\n");
    }
	std::cout << "Parsed command from client " << clientFd << ": [" << cmd << "] Full line: [" << command << "]" << std::endl;
}


void Server::sendMsg(int clientFd, const std::string& msg) {
	send(clientFd, msg.c_str(), msg.size(), 0);
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
