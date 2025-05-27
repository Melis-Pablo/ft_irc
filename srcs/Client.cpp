#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), username(""), realname(""), hostname(""), authenticated(false), registered(false), buffer("") {};
        
Client::~Client(){};

bool Client::isFullyRegistered() const {
    return authenticated && !nickname.empty() && !username.empty();
}

void Client::joinChannel(const std::string& channel_name) {
    channels.insert(channel_name);
}

void Client::leaveChannel(const std::string& channel_name) {
    channels.erase(channel_name);
}

bool Client::isInChannel(const std::string& channel_name) const {
    return channels.find(channel_name) != channels.end();
}