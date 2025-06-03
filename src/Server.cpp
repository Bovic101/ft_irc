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

    buffer[bytesRead] = '\0';

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
    } else if (cmd == "PRIVMSG") {
        if (_clients[clientFd].getNickname().empty()) {
            sendMsg(clientFd, "ERROR: You must set a nickname first\r\n");
            return;
        }
        if (args.size() < 2) {
            sendMsg(clientFd, "ERROR: PRIVMSG requires a target and a message\r\n");
            return;
        const std::string& target = args[0];
        const std::string& message = args[1];

        if (target[0] == '#') {
            std::map<std::string, Channel>::iterator channelIt = _channels.find(target);
            if (channelIt == _channels.end()) {
                sendMsg(clientFd, "ERROR: Channel " + target + " does not exist\r\n");
                return;
            }
            Channel& channel = channelIt->second;
            if (!channel.checkUser(clientFd)) {
                sendMsg(clientFd, "ERROR: You are not in channel " + target + "\r\n");
                return;
            }
            for (int fd : channel.getMembers()) {
                if (fd != clientFd) {
                    sendMsg(fd, _clients[clientFd].getNickname() + ": " + message + "\r\n");
                }
            }
        } else {
            auto it = _clients.find(clientFd);
            if (it == _clients.end()) {
                sendMsg(clientFd, "ERROR: You are not connected\r\n");
                return;
            }
            Client& sender = it->second;

            bool found = false;
            for (const auto& pair : _clients) {
                if (pair.second.getNickname() == target) {
                    sendMsg(pair.first, sender.getNickname() + ": " + message + "\r\n");
                    found = true;
                    break;
                }
            }
            if (!found) {
                sendMsg(clientFd, "ERROR: No such user: " + target + "\r\n");
            }
        }
    } else if (cmd == "INVITE"){
        if (args.size() < 2) {
            sendMsg(clientFd, "ERROR: INVITE requires a user and a channel\r\n");
            return;
        }
        const std::string& targetUser = args[0];
        const std::string& targetChannel = args[1];
        if (targetChannel[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }
        if (_channels.find(targetChannel) == _channels.end()) {
            sendMsg(clientFd, "ERROR: Channel " + targetChannel + " does not exist\r\n");
            return;
        }
        Channel& channel = _channels[targetChannel];
        if (!channel.checkOperator(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not a channel operator\r\n");
            return;
        }
        if (!channel.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not in channel " + targetChannel + "\r\n");
            return;
        }
        int targetFd = -1;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second.getNickname() == targetUser) {
                targetFd = it->first;
                break;
            }
        }
        if (targetFd == -1) {
            sendMsg(clientFd, "ERROR: User " + targetUser + " not found\r\n");
            return;
        }
        channel.inviteUser(targetFd);
        sendMsg(clientFd, "Invited " + targetUser + " to channel " + targetChannel + "\r\n");
        sendMsg(targetFd, ":" + _clients[clientFd].getNickname() + " INVITE " + targetUser + " " + targetChannel + "\r\n");
    } else if (cmd == "PART") {
        if (args.empty()) {
            sendMsg(clientFd, "ERROR: PART requires a channel name\r\n");
            return;
        }
        const std::string& channelName = args[0];
        if (channelName[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }
        if (_channels.find(channelName) == _channels.end()) {
            sendMsg(clientFd, "ERROR: Channel " + channelName + " does not exist\r\n");
            return;
        }
        Channel& channel = _channels[channelName];
        if (!channel.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not in channel " + channelName + "\r\n");
            return;
        }
        channel.userDeletion(clientFd);
        _clients[clientFd].partChannel(channelName);
        sendMsg(clientFd, "You have left channel: " + channelName + "\r\n");
        for (int fd : channel.getMembers()) {
            if (fd != clientFd) {
                sendMsg(fd, _clients[clientFd].getNickname() + " has left the channel\r\n");
            }
        }
        if (channel.totalMemberNum() == 0) {
            _channels.erase(channelName);
            std::cout << "Channel " << channelName << " has been deleted as it is empty." << std::endl;
        }
    } else if (cmd == "TOPIC") {
        if (args.empty()) {
            sendMsg(clientFd, "ERROR: TOPIC requires a channel name\r\n");
            return;
        }
        const std::string& channelName = args[0];
        if (channelName[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }
        if (_channels.find(channelName) == _channels.end()) {
            sendMsg(clientFd, "ERROR: Channel " + channelName + " does not exist\r\n");
            return;
        }
        Channel& channel = _channels[channelName];
        if (!channel.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not in channel " + channelName + "\r\n");
            return;
        }
        if (args.size() == 1) {
            const std::string& currentTopic = channel.currentTopic();
            if (currentTopic.empty()) {
                sendMsg(clientFd, "No topic set for channel " + channelName + "\r\n");
            } else {
                sendMsg(clientFd, "Current topic for channel " + channelName + ": " + currentTopic + "\r\n");
            }
        return;
        }
        std::string newTopic = args[1];
        for (size_t i = 2; i < args.size(); ++i) {
            newTopic += " " + args[i];
        }
        channel.topicSetter(newTopic);
        sendMsg(clientFd, "Topic for channel " + channelName + " set to: " + newTopic + "\r\n");
        for (int fd : channel.getMembers()) {
            if (fd != clientFd) {
                sendMsg(fd, "Topic for channel " + channelName + " has been changed to: " + newTopic + "\r\n");
            } else {
                sendMsg(clientFd, "ERROR: Unknown command: " + cmd + "\r\n");
                return;
            }
        }
        
    } else if (cmd == "MODE") {
        if (args.empty()) {
            sendMsg(clientFd, "ERROR: Usage: MODE <#channel> [+-mode] [param]\r\n");
            return;
        }
        const std::string& channelName = args[0];
        if (channelName[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }
        if (_channels.find(channelName) == _channels.end()) {
            sendMsg(clientFd, "ERROR: Channel " + channelName + " does not exist\r\n");
            return;
        }
        Channel& channel = _channels[channelName];
        if (!channel.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not in channel " + channelName + "\r\n");
            return;
        }
        if (args.size() == 1) {
            std::string modes = "Current modes for channel " + channelName + ": ";
            if (channel.isModeEnabled('i')) {
                modes += "i (invite-only) ";
            }
            if (channel.isModeEnabled('t')) {
                modes += "t (topic-protected) ";
            }
            if (channel.isModeEnabled('k')) {
                modes += "k (key-protected) ";
            }
            if (channel.isModeEnabled('l')) {
                modes += "l (user limit) ";
            }
            if (modes == "Current modes for channel " + channelName + ": ") {
                modes += "None";
            }
            sendMsg(clientFd, modes + "\r\n");
            return;
        }
        const std::string& modeFlag = args[1];
        if ( modeFlag.size() < 2 || (modeFlag[0] != '+' && modeFlag[0] != '-')) {
            sendMsg(clientFd, "ERROR: Invalid mode format. Use +i/-i, +l/-l, +k/-k\r\n");
            return;
        }
        char sign = modeFlag[0];
        char mode = modeFlag[1];
        if (!channel.checkOperator(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not a channel operator\r\n");
            return;
        }
        if (sign == '+') {
            if (mode == 'i') {
                channel.enableMode('i');
                sendMsg(clientFd, "Channel " + channelName + " is now invite-only\r\n");
            } else if (mode == 't') {
                channel.enableMode('t');
                sendMsg(clientFd, "Channel " + channelName + " topic is now protected\r\n");
            } else if (mode == 'k') {
                if (args.size() < 3) {
                    sendMsg(clientFd, "ERROR: MODE k requires a key\r\n");
                    return;
                }
                const std::string& key = args[2];
                channel.enableKey(key);
                sendMsg(clientFd, "Channel " + channelName + " key set to: " + key + "\r\n");
            } else if (mode == 'l') {
                if (args.size() < 3 || !std::isdigit(args[2][0])) {
                    sendMsg(clientFd, "ERROR: MODE l requires a numeric limit\r\n");
                    return;
                }
                int limit = std::stoi(args[2]);
                channel.limitSetter(limit);
                sendMsg(clientFd, "Channel " + channelName + " user limit set to: " + std::to_string(limit) + "\r\n");
            } else {
                sendMsg(clientFd, "ERROR: Unknown mode: +" + std::string(1, mode) + "\r\n");
            }
        } else if (sign == '-') {
            if (mode == 'i') {
                channel.disableMode('i');
                sendMsg(clientFd, "Channel " + channelName + " is no longer invite-only\r\n");
            } else if (mode == 't') {
                channel.disableMode('t');
                sendMsg(clientFd, "Channel " + channelName + " topic protection removed\r\n");
            } else if (mode == 'k') {
                channel.disableMode('k');
                sendMsg(clientFd, "Channel " + channelName + " key protection removed\r\n");
            } else if (mode == 'l') {
                channel.disableMode('l');
                sendMsg(clientFd, "Channel " + channelName + " user limit removed\r\n");
            } else {
                sendMsg(clientFd, "ERROR: Unknown mode: -" + std::string(1, mode) + "\r\n");
            }
        } else {
            sendMsg(clientFd, "ERROR: Invalid mode sign. Use + or -\r\n");
            return;
        }
    } else if (cmd == "KICK"){
        if (args.size() < 2) {
            sendMsg(clientFd, "ERROR: KICK requires a user and a channel\r\n");
            return;
        }
        const std::string& targetUser = args[0];
        const std::string& targetChannel = args[1];
        if (targetChannel[0] != '#') {
            sendMsg(clientFd, "ERROR: Channel name must start with #\r\n");
            return;
        }
        if (_channels.find(targetChannel) == _channels.end()) {
            sendMsg(clientFd, "ERROR: Channel " + targetChannel + " does not exist\r\n");
            return;
        }
        Channel& channel = _channels[targetChannel];
        if (!channel.checkUser(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not in channel " + targetChannel + "\r\n");
            return;
        }
        if (!channel.checkOperator(clientFd)) {
            sendMsg(clientFd, "ERROR: You are not a channel operator\r\n");
            return;
        }
        int targetFd = -1;
        for (const auto& pair : _clients) {
            if (pair.second.getNickname() == targetUser) {
                targetFd = pair.first;
                break;
            }
        }
        if (targetFd == -1) {
            sendMsg(clientFd, "ERROR: User " + targetUser + " not found\r\n");
            return;
        }
        if (!channel.checkUser(targetFd)) {
            sendMsg(clientFd, "ERROR: User " + targetUser + " is not in channel " + targetChannel + "\r\n");
            return;
        }
        channel.kickUser(targetFd);
        _clients[targetFd].partChannel(targetChannel);
        sendMsg(clientFd, "Kicked " + targetUser + " from channel " + targetChannel + "\r\n");
        for (int fd : channel.getMembers()) {
            if (fd != clientFd && fd != targetFd) {
                sendMsg(fd, _clients[clientFd].getNickname() + " has kicked " + targetUser + " from the channel\r\n");
            }
        }
    }else if (cmd == "QUIT"){
        std::string quitMessage = args.empty() ? "Client disconnected" : args[0];
        for (const std::string& channelName : _clients[clientFd].getChannels()) {
            Channel& channel = _channels[channelName];
            for (int fd : channel.getMembers()) {
                if (fd != clientFd) {
                    sendMsg(fd, _clients[clientFd].getNickname() + " has left the channel: " + channelName + "\r\n");
                }
            }
            channel.userDeletion(clientFd);
        }
        sendMsg(clientFd, "GOODBYE!: " + quitMessage + "\r\n");
        _clients.erase(clientFd);
        removeFromPollfd(clientFd);
        close(clientFd);
        std::cout << "Client " << clientFd << " disconnected: " << quitMessage << std::endl;
    } else {
        sendMsg(clientFd, "ERROR: Unknown command: " + cmd + "\r\n");
        std::cout << "Unknown command received from client " << clientFd << ": " << cmd << std::endl;
        return;
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

