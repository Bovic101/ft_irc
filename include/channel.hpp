#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <string>
#include <map>
#include <set>

class Channel
{
    private:
        std::string _channelName;
        std::string _channelTopic;
        std::string _channelKey; 
        std::set<int> _members; //FD of client in the channel
        std::set<int> _channelOperator;
        std::set<int> _invitedClient; //The client invited for option i
        std::set<char> _ChannelMode; //Too change channel,option(i, t, k o, l)
        int _channeluserLimit;
    
    public:
        Channel(); //default constructor
        explicit Channel(const std::string& name);
        Channel(const Channel& other);//copy constructor
        Channel& operator=(const Channel& other);//copy assignment operator
        ~Channel();

        //Method for User
        void userAddition(int fd); //To add client to the channe
        void userDeletion(int fd);
        int totalMemberNum() const; //To return total num of user in the channel
        bool checkUser(int fd); //To check if the users belong to channel
        std::set<int>& getMembers(); //To return all the users in the channel

        //Method for channel Topic
        void topicSetter(const std::string& topic);
        const std::string& currentTopic() const; //To return current topic in channel
        
        //Method for Channel Operator
        void giveOperator(int fd); //To give operator status to user
        void revokeOperator(int fd); //Revoke operator status
        bool checkOperator(int fd) const; // To check if the user has operator status

        //Method for invitation of user
        void inviteUser(int fd); //To add user by invite
        void deleteInvite(int fd); 
        bool checkInvite(int fd); //To check if the user has been invited

        //Method for channel key 
        void enableKey(const std::string& key); //To enable key
        bool validateKey(const std::string& key) const; //This compare key to authenticate

        //Method for Channel User limit
        void limitSetter(int num); // This set channel user limit
        bool validateLimit() const; //Check if user count is greater than or = limit

        //Method for channel mode
        void enableMode(char setmode); //Enable channel mode
        void disableMode(char setmode);
        bool isModeEnabled(char mode) const;
};
#endif
