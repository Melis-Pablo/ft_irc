#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

class IRCMessage {
    public:
        std::string prefix;
        std::string command;
        std::vector<std::string> params;
        std::string trailing;
        
        // Constructor
        IRCMessage() : prefix(""), command(""), trailing("") {}
        
        // Debug output
        void print_debug();
};

// Function to parse IRC messages
IRCMessage parseMessage(const std::string& raw_message);

#endif