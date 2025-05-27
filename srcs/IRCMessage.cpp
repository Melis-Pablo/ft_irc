#include "IRCMessage.hpp"

IRCMessage parseMessage(const std::string& raw_message) {
    IRCMessage msg;
    std::string line = raw_message;
    
    // Remove \r\n at the end
    if(!line.empty() && line[line.length() - 1] == '\n') 
        line.erase(line.length() - 1);
    if(!line.empty() && line[line.length() - 1] == '\r') 
        line.erase(line.length() - 1);
    
    if(line.empty()) {
        return msg;  // Empty message
    }
    
    size_t pos = 0;
    
    // Step 1: Parse prefix (optional, starts with ':')
    if(line[0] == ':') {
        size_t space_pos = line.find(' ');
        if(space_pos != std::string::npos) {
            msg.prefix = line.substr(1, space_pos - 1);  // Skip ':'
            pos = space_pos + 1;  // Continue after space
        } else {
            // No spaces found, entire line is prefix (shouldn't happen)
            msg.prefix = line.substr(1);
            return msg;
        }
    }
    
    // Step 2: Find and separate trailing (optional, ' :' pattern)
    size_t trailing_pos = line.find(" :", pos);
    std::string params_part;
    
    if(trailing_pos != std::string::npos) {
        params_part = line.substr(pos, trailing_pos - pos);
        msg.trailing = line.substr(trailing_pos + 2);  // Skip " :"
    } else {
        params_part = line.substr(pos);
        msg.trailing = "";
    }
    
    // Step 3: Extract command and params from params_part
    std::istringstream iss(params_part);
    std::string word;
    bool first_word = true;
    
    while(iss >> word) {
        if(first_word) {
            msg.command = word;
            first_word = false;
        } else {
            msg.params.push_back(word);
        }
    }
    
    return msg;
}

void IRCMessage::print_debug() {
        std::cout << "Prefix: '" << prefix << "'" << std::endl;
        std::cout << "Command: '" << command << "'" << std::endl;
        std::cout << "Params: ";
        for(size_t i = 0; i < params.size(); i++) {
            std::cout << "'" << params[i] << "'";
            if(i < params.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "Trailing: '" << trailing << "'" << std::endl;
}