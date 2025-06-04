#include "Client.hpp"

Client::Client()
    : _fd(-1), _buffer(""), _nickname(""),
      registeredChecker(false), authenticatedChecker(false) {}

Client::Client(int fd)
    : _fd(fd), _buffer(""), _nickname(""),
      registeredChecker(false), authenticatedChecker(false) {}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

void Client::appendBuffer(const std::string& data) {
    _buffer += data;
}

std::string& Client::getBuffer() {
    return _buffer;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

void Client::addChannel(const std::string& channel) {
    _channels.insert(channel);
}

void Client::partChannel(const std::string& channel) {
    _channels.erase(channel);
}

bool Client::isInChannel(const std::string& channel) const {
    return _channels.find(channel) != _channels.end();
}

const std::set<std::string>& Client::getChannels() const {
    return _channels;
}

bool Client::registeredCheckerFunc() const {
    return registeredChecker;
}

bool Client::authenticatedCheckerFunc() const {
    return authenticatedChecker;
}

void Client::grantAuth(bool status) {
    authenticatedChecker = status;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::markAsRegistered(bool status) {
    registeredChecker = status;
}