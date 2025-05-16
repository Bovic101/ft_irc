#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <cstring>

class Client {
public:
	Client(int fd);
	~Client();

	int getFd() const;
	void appendBuffer(const std::string& data);
	std::string &getBuffer();

private:
	int _fd;
	std::string _buffer;
};
#endif