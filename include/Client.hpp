#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class Client {
    public:
        int fd;
        std::string nickname;
        std::string username;
        std::string realname;
        std::string hostname;
        bool authenticated;
        bool registered;
        std::string buffer;
        std::set<std::string> channels; // Channels this client has joined

        Client();
        ~Client();

        bool isFullyRegistered() const;
        
        // Channel management
        void joinChannel(const std::string& channel_name);
        void leaveChannel(const std::string& channel_name);
        bool isInChannel(const std::string& channel_name) const;
        const std::set<std::string>& getChannels() const { return channels; }
};

#endif