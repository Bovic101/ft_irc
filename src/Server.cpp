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
    fcntl(_serverSocket, F_SETFL, O_NONBLOCK);

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

    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    _clients.emplace(clientFd, Client(clientFd));

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
        removeClient(clientFd);
        return;
    }

    buffer[bytesRead] = '\0'; // ✅ fixed here

    auto it = _clients.find(clientFd);
    if (it == _clients.end()) {
        auto result = _clients.emplace(clientFd, Client(clientFd));
        it = result.first;
    }
    it->second.appendBuffer(buffer);

    std::string& clientBuffer = it->second.getBuffer();
    std::vector<std::string> fullLines = InputParser::XtractInput(clientBuffer);

    for (const std::string& line : fullLines) {
        parseCommand(clientFd, line);
    }
}

void Server::parseCommand(int clientFd, const std::string& line) {
    ParseCmd parsed = InputParser::parseCommand(line);
    const std::string& cmd = parsed.command;
    const std::vector<std::string>& args = parsed.args;

    if (cmd == "NICK") {
        if (args.empty()) {
            sendMsg(clientFd, "ERROR: Nickname cannot be empty\r\n");
            return;
        }
        const std::string& nickname = args[0];

        for (const auto& pair : _clients) {
            if (pair.second.getNickname() == nickname) {
                sendMsg(clientFd, "ERROR: Nickname already in use\r\n");
                return;
            }
        }
        _clients[clientFd].setNickname(nickname);
        sendMsg(clientFd, "Nickname set to: " + nickname + "\r\n");

    } else if (cmd == "JOIN") {
        if (_clients[clientFd].getNickname().empty()) {
            sendMsg(clientFd, "ERROR: You must set a nickname first\r\n");
            return;
        }
        if (args.empty()) {
            sendMsg(clientFd, "ERROR: Channel cannot be empty\r\n");
            return;
        }
        const std::string& channel = args[0];
        if (channel[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }

        Channel& ch = _channels[channel];
        if (ch.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: Already in channel " + channel + "\r\n");
            return;
        }
        if (ch.isModeEnabled('i') && !ch.checkInvite(clientFd)) {
            sendMsg(clientFd, "ERROR: Channel " + channel + " is invite-only\r\n");
            return;
        }
        if (!ch.validateLimit()) {
            sendMsg(clientFd, "ERROR: Channel " + channel + " is full\r\n");
            return;
        }

        ch.userAddition(clientFd);
        _clients[clientFd].addChannel(channel);
        ch.deleteInvite(clientFd);
        if (ch.totalMemberNum() == 1) {
            ch.giveOperator(clientFd);
        }
        sendMsg(clientFd, "Joined channel: " + channel + "\r\n");

        for (int fd : ch.getMembers()) {
            if (fd != clientFd) {
                sendMsg(fd, _clients[clientFd].getNickname() + " has joined the channel\r\n");
            }
        }
    }

    std::cout << "Parsed command from client " << clientFd << ": [" << cmd << "] Full line: [" << line << "]" << std::endl;
}

void Server::sendMsg(int clientFd, const std::string& msg) {
    send(clientFd, msg.c_str(), msg.size(), 0);
}

void Server::removeFromPollfd(int clientFd) {
    for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == clientFd) {
            _pollfds.erase(it);
            break;
        }
    }
}

void Server::removeClient(int clientFd) {
    close(clientFd);
    _clients.erase(clientFd);
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == clientFd) {
            _pollfds.erase(_pollfds.begin() + i);
            break;
        }
    }
    std::cout << "Client disconnected! FD: " << clientFd << std::endl;
}

