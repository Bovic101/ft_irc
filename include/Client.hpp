#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <cstring>
#include <set>
#include <string>

class Client {
public:
	Client(); //added def constructor
	Client(int fd);

	~Client();

	int getFd() const;
	void appendBuffer(const std::string& data);
	std::string &getBuffer();

	void setNickname(const std::string& nickname);
	const std::string& getNickname() const;
	void addChannel(const std::string& channel);
	void partChannel(const std::string& channel);
	bool isInChannel(const std::string& channel) const;
	const std::set<std::string>& getChannels() const;


private:
	int _fd;
	std::string _buffer;
	std::string _nickname;
	std::set<std::string> _channels;
};
#endif