#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class Client {
private:
    int _fd;
    std::string _buffer;
    std::string _nickname;
    std::string _username; // added
    std::set<std::string> _channels;
    bool registeredChecker;
    bool authenticatedChecker;

public:
    Client();
    Client(int fd);
    ~Client();

    int getFd() const;
    void appendBuffer(const std::string& data);
    std::string& getBuffer();
    void setNickname(const std::string& nickname);
    const std::string& getNickname() const;

    void addChannel(const std::string& channel);
    void partChannel(const std::string& channel);
    bool isInChannel(const std::string& channel) const;
    const std::set<std::string>& getChannels() const;

    //Added methods for registration and authentication
    bool registeredCheckerFunc() const;
    bool authenticatedCheckerFunc() const;
    void grantAuth(bool status);
    void setUsername(const std::string& username); // added
    void markAsRegistered(bool status); 
};

#endif // CLIENT_HPP
