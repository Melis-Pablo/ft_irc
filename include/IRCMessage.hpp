#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <string> // For std::string
#include <vector> // For std::vector
#include <iostream> // For std::cout
#include <sstream> // For std::istringstream

class IRCMessage {
    public:
        std::string prefix;
        std::string command;
        std::vector<std::string> params;
        std::string trailing;
        
        IRCMessage();
        ~IRCMessage();
};

IRCMessage parseMessage(const std::string& raw_message);

#endif