#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <string>
#include <map>
#include <set>

/**
 * @class Channel
 * @brief Represents an IRC channel and its properties.
 */
class Channel {
private:
    std::string _channelName;
    std::string _channelTopic;
    std::string _channelKey;
    std::set<int> _members;
    std::set<int> _channelOperator;
    std::set<int> _invitedClient;
    std::set<char> _ChannelMode; // Mode flags: i, t, k, l, o, etc.
    int _channeluserLimit;

public:
    Channel();                                      // Default constructor
    explicit Channel(const std::string& name);      // Constructor with name
    Channel(const Channel& other);                  // Copy constructor
    Channel& operator=(const Channel& other);       // Assignment operator
    ~Channel();                                     // Destructor

    // Member handling
    void userAddition(int fd);
    void userDeletion(int fd);
    int totalMemberNum() const;
    bool checkUser(int fd) const;
    const std::set<int>& getMembers() const;

    // Topic
    void topicSetter(const std::string& topic);
    const std::string& currentTopic() const;

    // Operators
    void giveOperator(int fd);
    void revokeOperator(int fd);
    bool checkOperator(int fd) const;

    // Invitations
    void inviteUser(int fd);
    void deleteInvite(int fd);
    bool checkInvite(int fd) const;

    // Key
    void enableKey(const std::string& key);
    bool validateKey(const std::string& key) const;

    // User limit
    void limitSetter(int num);
    bool validateLimit() const;

    // Modes
    void enableMode(char setmode);
    void disableMode(char setmode);
    bool isModeEnabled(char mode) const;
};

#endif // CHANNEL_HPP
