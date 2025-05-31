#include "channel.hpp"

// Default constructor
Channel::Channel() 
    : _channelName(""), _channelTopic(""), _channeluserLimit(-1) {}

// Constructor with channel name
Channel::Channel(const std::string& name) 
    : _channelName(name), _channelTopic(""), _channeluserLimit(-1) {}

// Copy constructor
Channel::Channel(const Channel& other)
    : _channelName(other._channelName), _channelTopic(other._channelTopic),
      _channelKey(other._channelKey), _members(other._members),
      _channelOperator(other._channelOperator), _invitedClient(other._invitedClient),
      _ChannelMode(other._ChannelMode), _channeluserLimit(other._channeluserLimit) {}

// Assignment operator
Channel& Channel::operator=(const Channel& other) {
    if (this != &other) {
        _channelName = other._channelName;
        _channelTopic = other._channelTopic;
        _channelKey = other._channelKey;
        _members = other._members;
        _channelOperator = other._channelOperator;
        _invitedClient = other._invitedClient;
        _ChannelMode = other._ChannelMode;
        _channeluserLimit = other._channeluserLimit;
    }
    return *this;
}

// Destructor
Channel::~Channel() = default;

// Add a user
void Channel::userAddition(int fd) {
    _members.insert(fd);
}

// Remove a user
void Channel::userDeletion(int fd) {
    _members.erase(fd);
    _channelOperator.erase(fd);
    _invitedClient.erase(fd);
}

// Get number of users
int Channel::totalMemberNum() const {
    return static_cast<int>(_members.size());
}

// Check membership
bool Channel::checkUser(int fd) const {
    return _members.find(fd) != _members.end();
}

// Return all members
const std::set<int>& Channel::getMembers() const {
    return _members;
}

// Set topic
void Channel::topicSetter(const std::string& topic) {
    _channelTopic = topic;
}

// Get current topic
const std::string& Channel::currentTopic() const {
    return _channelTopic;
}

// Add operator
void Channel::giveOperator(int fd) {
    _channelOperator.insert(fd);
}

// Remove operator
void Channel::revokeOperator(int fd) {
    _channelOperator.erase(fd);
}

// Check if user is operator
bool Channel::checkOperator(int fd) const {
    return _channelOperator.find(fd) != _channelOperator.end();
}

// Invite a user
void Channel::inviteUser(int fd) {
    _invitedClient.insert(fd);
}

// Remove invite
void Channel::deleteInvite(int fd) {
    _invitedClient.erase(fd);
}

// Check if user is invited
bool Channel::checkInvite(int fd) const {
    return _invitedClient.find(fd) != _invitedClient.end();
}

// Set channel key
void Channel::enableKey(const std::string& key) {
    _channelKey = key;
    enableMode('k');
}

// Check key
bool Channel::validateKey(const std::string& key) const {
    return _channelKey == key;
}

// Set limit
void Channel::limitSetter(int num) {
    _channeluserLimit = num;
    enableMode('l');
}

// Validate limit
bool Channel::validateLimit() const {
    return _channeluserLimit < 0 || static_cast<int>(_members.size()) <= _channeluserLimit;
}

// Enable mode
void Channel::enableMode(char setmode) {
    _ChannelMode.insert(setmode);
}

// Disable mode
void Channel::disableMode(char setmode) {
    _ChannelMode.erase(setmode);
}

// Check if mode is enabled
bool Channel::isModeEnabled(char mode) const {
    return _ChannelMode.find(mode) != _ChannelMode.end();
}

