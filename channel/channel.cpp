#include "channel.hpp"

//Default constructor-initializes the channel with  -1 have no limit
Channel::Channel() : _channelName(""), _channelTopic(""), _channeluserLimit(-1) {}
//Constructor with channel name
Channel::Channel(const std::string& name) 
    : _channelName(name), _channelTopic(""), _channeluserLimit(-1) {}
//Copy constructor
Channel::Channel(const Channel& other) 
    : _channelName(other._channelName), _channelTopic(other._channelTopic), 
      _channelKey(other._channelKey), _members(other._members), 
      _channelOperator(other._channelOperator), _invitedClient(other._invitedClient), 
      _ChannelMode(other._ChannelMode), _channeluserLimit(other._channeluserLimit) {}
//Copy assignment operator
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
//Destructor
Channel::~Channel() = default;
//Method to add user to the channel
void Channel::userAddition(int fd) {
    _members.insert(fd);
}
//Method to delete user from the channel
void Channel::userDeletion(int fd) {
    _members.erase(fd);
    -_channelOperator.erase(fd);
    _invitedClient.erase(fd);
}
//Method to return total number of users in the channel
int Channel::totalMemberNum() const {
    return _members.size();
}
//Method to check if user belongs to the channel
bool Channel::checkUser(int fd) {
    return _members.find(fd) != _members.end();
}
//Method to return all users in the channel
std::set<int>& Channel::totalMemberNum() {
    return _members;
}
//Method to set the channel topic
void Channel::topicSetter(const std::string& topic) {
    _channelTopic = topic;
}
//Method to return current topic in the channel
const std::string& Channel::currentTopic() const {
    return _channelTopic;
}
//Method to give operator status to user
void Channel::giveOperator(int fd) {
    _channelOperator.insert(fd);
}
//Method to revoke operator status from user
void Channel::revokeOperator(int fd) {
    _channelOperator.erase(fd);
}
//Method to check if user has operator status
bool Channel::checkOperator(int fd) const {
    return _channelOperator.find(fd) != _channelOperator.end();
}
//Method to invite user to the channel
void Channel::inviteUser(int fd) {
    _invitedClient.insert(fd);
}
//Method to delete user from the invite list
void Channel::deleteInvite(int fd) {
    _invitedClient.erase(fd);
}
//Method to check if user has been invited
bool Channel::checkInvite(int fd) {
    return _invitedClient.find(fd) != _invitedClient.end();
}
//Method to enable channel key
void Channel::enableKey(const std::string& key) {
    _channelKey = key;
    enableMode('k'); // Enable key mode
}
//Method to validate channel key
bool Channel::validateKey(const std::string& key) const {
    return _channelKey == key;
}
//Method to set channel user limit
void Channel::limitSetter(int num) {
    _channeluserLimit = num;
    enableMode('l'); // Enable limit mode
}
//Method to validate if user count is within limit
bool Channel::validateLimit() const {
    return _channeluserLimit < 0 || _members.size() <= _channeluserLimit;
}
//Method to enable channel mode
void Channel::enableMode(char setmode) {
    _ChannelMode.insert(setmode);
}
//Method to disable channel mode
void Channel::disableMode(char setmode) {
    _ChannelMode.erase(setmode);
}
