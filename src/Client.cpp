#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _buffer("") {}
Client::~Client() {
	
	
}

int Client::getFd() const {
	return _fd;
}

void Client::appendBuffer(const std::string& data) {
	_buffer += data;
}
std::string &Client::getBuffer() {
	return _buffer;
}

// Destructor to close the sock