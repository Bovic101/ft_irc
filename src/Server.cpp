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

    std::cout << "Custom Server is listening on port :" << _port << std::endl;
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

    buffer[bytesRead] = '\0';
    _clients[clientFd].appendBuffer(buffer);
    std::string& clientBuffer = _clients[clientFd].getBuffer();
    std::vector<std::string> fullLines = InputParser::XtractInput(clientBuffer);

    for (const std::string& line : fullLines)
        parseCommand(clientFd, line);
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

void Server::parseCommand(int clientFd, const std::string& line) {
    ParseCmd parsed = InputParser::parseCommand(line);
    const std::string& cmd = parsed.command;
    const std::vector<std::string>& args = parsed.args;

    if (cmd == "NICK") {
        if (args.empty()) return sendMsg(clientFd, "ERROR: Nickname cannot be empty\r\n");
        const std::string& nick = args[0];
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.getNickname() == nick)
                return sendMsg(clientFd, "ERROR: Nickname already in use\r\n");
        }
        _clients[clientFd].setNickname(nick);
        sendMsg(clientFd, "Nickname set to: " + nick + "\r\n");
    }

    else if (cmd == "JOIN") {
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
    else if (cmd == "LIST") {
        std::string response = "Channels:\r\n";
        for (const auto& pair : _channels) {
            const Channel& ch = pair.second;
            response += ch.getChannelName() + " (" + std::to_string(ch.totalMemberNum()) + " members)\r\n";
        }
        sendMsg(clientFd, response);
    }

    else if (cmd == "INVITE") {
        if (args.size() < 2) return sendMsg(clientFd, "ERROR: INVITE requires a user and a channel\r\n");
        const std::string& targetUser = args[0];
        const std::string& chName = args[1];

        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel " + chName + " not found\r\n");

        Channel& ch = _channels[chName];
        if (!ch.checkOperator(clientFd))
            return sendMsg(clientFd, "ERROR: You are not an operator of the channel\r\n");

        int targetFd = -1;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.getNickname() == targetUser) {
                targetFd = it->first;
                break;
            }
        }
        if (targetFd == -1)
            return sendMsg(clientFd, "ERROR: User not found\r\n");

        ch.inviteUser(targetFd);
        sendMsg(targetFd, "You have been invited to join channel " + chName + "\r\n");
    }

    else if (cmd == "INVITE") {
        if (args.size() < 2) return sendMsg(clientFd, "ERROR: INVITE requires a user and a channel\r\n");
        const std::string& targetUser = args[0];
        const std::string& chName = args[1];

        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel " + chName + " not found\r\n");

        Channel& ch = _channels[chName];
        if (!ch.checkOperator(clientFd))
            return sendMsg(clientFd, "ERROR: You are not an operator of the channel\r\n");

        int targetFd = -1;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.getNickname() == targetUser) {
                targetFd = it->first;
                break;
            }
        }
        if (targetFd == -1)
            return sendMsg(clientFd, "ERROR: User not found\r\n");

        ch.inviteUser(targetFd);
        sendMsg(targetFd, "You have been invited to join channel " + chName + "\r\n");
    }

    else if (cmd == "PRIVMSG") {
        if (args.size() < 2) return sendMsg(clientFd, "ERROR: PRIVMSG requires a target and a message\r\n");
        const std::string& target = args[0];
        std::string message = args[1];
        for (size_t i = 2; i < args.size(); ++i)
            message += " " + args[i];

        if (target[0] == '#') {
            if (_channels.find(target) == _channels.end())
                return sendMsg(clientFd, "ERROR: Channel " + target + " not found\r\n");
            Channel& ch = _channels[target];
            if (!ch.checkUser(clientFd))
                return sendMsg(clientFd, "ERROR: You are not in channel " + target + "\r\n");
            for (int fd : ch.getMembers()) {
                if (fd != clientFd)
                    sendMsg(fd, _clients[clientFd].getNickname() + ": " + message + "\r\n");
            }
        } else {
            for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
                if (it->second.getNickname() == target) {
                    sendMsg(it->first, _clients[clientFd].getNickname() + ": " + message + "\r\n");
                    return;
                }
            }
            sendMsg(clientFd, "ERROR: No such user\r\n");
        }
    }

    else if (cmd == "PART") {
        if (args.empty()) return sendMsg(clientFd, "ERROR: PART requires a channel name\r\n");
        const std::string& chName = args[0];
        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel " + chName + " not found\r\n");
        Channel& ch = _channels[chName];
        if (!ch.checkUser(clientFd))
            return sendMsg(clientFd, "ERROR: You are not in the channel\r\n");

        ch.userDeletion(clientFd);
        _clients[clientFd].partChannel(chName);
        for (int fd : ch.getMembers()) {
            sendMsg(fd, _clients[clientFd].getNickname() + " has left the channel\r\n");
        }
    }

    else if (cmd == "TOPIC") {
        if (args.empty()) return sendMsg(clientFd, "ERROR: TOPIC requires a channel name\r\n");
        const std::string& chName = args[0];
        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel " + chName + " not found\r\n");
        Channel& ch = _channels[chName];
        if (!ch.checkUser(clientFd))
            return sendMsg(clientFd, "ERROR: You are not in the channel\r\n");

        if (args.size() == 1) {
            const std::string& t = ch.currentTopic();
            sendMsg(clientFd, t.empty() ? "No topic set\r\n" : "Topic: " + t + "\r\n");
        } else {
            std::string newTopic = args[1];
            for (size_t i = 2; i < args.size(); ++i)
                newTopic += " " + args[i];
            ch.topicSetter(newTopic);
            for (int fd : ch.getMembers())
                sendMsg(fd, "Channel topic changed to: " + newTopic + "\r\n");
        }
    }

    else if (cmd == "MODE") {
        if (args.size() < 2)
            return sendMsg(clientFd, "ERROR: MODE <#channel> <+mode|-mode> [param]\r\n");
        const std::string& chName = args[0];
        std::string mode = args[1];
        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel " + chName + " does not exist\r\n");

        Channel& ch = _channels[chName];
        if (!ch.checkOperator(clientFd))
            return sendMsg(clientFd, "ERROR: You are not operator\r\n");

        char sign = mode[0];
        char flag = mode[1];
        if (sign == '+') {
            if (flag == 'i') ch.enableMode('i');
            else if (flag == 't') ch.enableMode('t');
            else if (flag == 'k') ch.enableKey(args.size() >= 3 ? args[2] : "");
            else if (flag == 'l') ch.limitSetter(args.size() >= 3 ? std::atoi(args[2].c_str()) : -1);
        } else if (sign == '-') {
            if (flag == 'i') ch.disableMode('i');
            else if (flag == 't') ch.disableMode('t');
            else if (flag == 'k') ch.revokeKey();
            else if (flag == 'l') ch.disableMode('l');
        } else {
            sendMsg(clientFd, "ERROR: Invalid mode\r\n");
        }
    }

    else if (cmd == "KICK") {
        if (args.size() < 2)
            return sendMsg(clientFd, "ERROR: KICK <user> <#channel>\r\n");
        const std::string& targetUser = args[0];
        const std::string& chName = args[1];
        if (_channels.find(chName) == _channels.end())
            return sendMsg(clientFd, "ERROR: Channel not found\r\n");

        Channel& ch = _channels[chName];
        if (!ch.checkOperator(clientFd))
            return sendMsg(clientFd, "ERROR: You are not operator\r\n");

        int targetFd = -1;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.getNickname() == targetUser) {
                targetFd = it->first;
                break;
            }
        }
        if (targetFd == -1)
            return sendMsg(clientFd, "ERROR: User not found\r\n");

        ch.kickUser(targetFd);
        _clients[targetFd].partChannel(chName);
        sendMsg(clientFd, "User kicked\r\n");
    }

    else if (cmd == "QUIT") {
        std::string quitMsg = args.empty() ? "Client quit" : args[0];
        for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
            Channel& ch = it->second;
            if (ch.checkUser(clientFd)) {
                ch.userDeletion(clientFd);
                for (int fd : ch.getMembers()) {
                    if (fd != clientFd)
                        sendMsg(fd, _clients[clientFd].getNickname() + " quit: " + quitMsg + "\r\n");
                }
            }
        }
        sendMsg(clientFd, "GOODBYE\r\n");
        removeClient(clientFd);
    }

    std::cout << "Parsed command from client " << clientFd << ": [" << cmd << "] Full line: [" << line << "]" << std::endl;
}
