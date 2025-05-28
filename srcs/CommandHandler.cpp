#include "CommandHandler.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstdlib>

CommandHandler::CommandHandler(Server* srv) : server(srv) {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::handleIRCMessage(int client_index, const IRCMessage& msg) {
    if (msg.command.empty()) {
        return;
    }

    // Convert command to uppercase for case-insensitive comparison
    std::string cmd = msg.command;
    for (size_t i = 0; i < cmd.length(); i++) {
        cmd[i] = std::toupper(cmd[i]);
    }

    if (cmd == "PASS") {
        handlePass(client_index, msg);
    } else if (cmd == "NICK") {
        handleNick(client_index, msg);
    } else if (cmd == "USER") {
        handleUser(client_index, msg);
    } else if (cmd == "PING") {
        handlePing(client_index, msg);
    } else if (cmd == "QUIT") {
        handleQuit(client_index, msg);
    } else if (cmd == "JOIN") {
        handleJoin(client_index, msg);
    } else if (cmd == "PART") {
        handlePart(client_index, msg);
    } else if (cmd == "PRIVMSG") {
        handlePrivmsg(client_index, msg);
    } else if (cmd == "KICK") {
        handleKick(client_index, msg);
    } else if (cmd == "INVITE") {
        handleInvite(client_index, msg);
    } else if (cmd == "TOPIC") {
        handleTopic(client_index, msg);
    } else if (cmd == "MODE") {
        handleMode(client_index, msg);
    } else if (cmd == "WHOIS") {
        handleWhois(client_index, msg);
    } else {
        // Unknown command
        std::vector<Client>& clients = server->getClients();
        if (clients[client_index].isFullyRegistered()) {
            server->sendMessage(clients[client_index].fd, "421 " + clients[client_index].nickname + " " + cmd + " :Unknown command");
        }
    }
}

void CommandHandler::handlePass(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "461 * PASS :Not enough parameters");
        return;
    }

    if (clients[client_index].authenticated) {
        server->sendMessage(clients[client_index].fd, "462 * :You may not reregister");
        return;
    }

    if (msg.params[0] == server->getPassword()) {
        clients[client_index].authenticated = true;
        std::cout << "Client " << clients[client_index].fd << " authenticated successfully" << std::endl;
    } else {
        server->sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        std::cout << "Client " << clients[client_index].fd << " failed authentication" << std::endl;
    }
}

void CommandHandler::handleNick(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "431 * :No nickname given");
        return;
    }

    // Check if client provided password first
    if (!clients[client_index].authenticated) {
        server->sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        return;
    }

    std::string new_nick = msg.params[0];
    
    // Check if nickname is already in use
    if (server->isNicknameInUse(new_nick, client_index)) {
        server->sendMessage(clients[client_index].fd, "433 * " + new_nick + " :Nickname is already in use");
        return;
    }

    std::string old_nick = clients[client_index].nickname;
    clients[client_index].nickname = new_nick;
    
    if (old_nick.empty()) {
        std::cout << "Client " << clients[client_index].fd << " set nickname to: " << new_nick << std::endl;
    } else {
        std::cout << "Client " << clients[client_index].fd << " changed nickname from " << old_nick << " to " << new_nick << std::endl;
    }

    // If client is now fully registered, send welcome messages
    if (clients[client_index].isFullyRegistered() && !clients[client_index].registered) {
        clients[client_index].registered = true;
        server->sendWelcomeMessages(client_index);
    }
}

void CommandHandler::handleUser(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    if (msg.params.size() < 3 || msg.trailing.empty()) {
        server->sendMessage(clients[client_index].fd, "461 * USER :Not enough parameters");
        return;
    }

    // Check if client provided password first
    if (!clients[client_index].authenticated) {
        server->sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        return;
    }

    if (!clients[client_index].username.empty()) {
        server->sendMessage(clients[client_index].fd, "462 * :You may not reregister");
        return;
    }

    clients[client_index].username = msg.params[0];
    clients[client_index].realname = msg.trailing;
    
    std::cout << "Client " << clients[client_index].fd << " set username to: " << clients[client_index].username 
              << " realname: " << clients[client_index].realname << std::endl;

    // If client is now fully registered, send welcome messages
    if (clients[client_index].isFullyRegistered() && !clients[client_index].registered) {
        clients[client_index].registered = true;
        server->sendWelcomeMessages(client_index);
    }
}

void CommandHandler::handlePing(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    std::string response = "PONG";
    if (!msg.params.empty()) {
        response += " :" + msg.params[0];
    } else if (!msg.trailing.empty()) {
        response += " :" + msg.trailing;
    }
    server->sendMessage(clients[client_index].fd, response);
}

void CommandHandler::handleQuit(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    std::string quit_msg = msg.trailing.empty() ? "Client Quit" : msg.trailing;
    std::cout << "Client " << clients[client_index].fd << " (" << clients[client_index].nickname 
              << ") quit: " << quit_msg << std::endl;
    // Note: Server will need to provide a method to remove clients, or we handle this differently
}

void CommandHandler::handleWhois(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "431 " + clients[client_index].nickname + " :No nickname given");
        return;
    }

    std::string target_nick = msg.params[0];
    int target_index = server->findClientByNickname(target_nick);
    
    if (target_index == -1) {
        server->sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        server->sendMessage(clients[client_index].fd, "318 " + clients[client_index].nickname + " " + target_nick + " :End of WHOIS list");
        return;
    }

    const Client& target = clients[target_index];
    
    // Send WHOIS information
    server->sendMessage(clients[client_index].fd, "311 " + clients[client_index].nickname + " " + target.nickname + " " + target.username + " " + target.hostname + " * :" + target.realname);
    server->sendMessage(clients[client_index].fd, "318 " + clients[client_index].nickname + " " + target.nickname + " :End of WHOIS list");
}

void CommandHandler::handleJoin(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " JOIN :Not enough parameters");
        return;
    }

    std::string channel_names = msg.params[0];
    std::string keys = "";
    if (msg.params.size() > 1) {
        keys = msg.params[1];
    }

    // Handle multiple channels separated by commas
    std::istringstream channels_stream(channel_names);
    std::istringstream keys_stream(keys);
    std::string channel_name, key;
    
    while (std::getline(channels_stream, channel_name, ',')) {
        std::getline(keys_stream, key, ',');
        
        if (!server->isValidChannelName(channel_name)) {
            server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
            continue;
        }

        // Create channel if it doesn't exist
        if (channels.find(channel_name) == channels.end()) {
            channels.insert(std::make_pair(channel_name, Channel(channel_name)));
        }

        Channel& channel = channels[channel_name];
        
        // Check if client can join
        if (!channel.canJoin(clients[client_index].fd, key)) {
            if (channel.getUserCount() >= channel.getUserLimit() && channel.hasUserLimit()) {
                server->sendMessage(clients[client_index].fd, "471 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+l)");
            } else if (channel.isInviteOnly() && !channel.isInvited(clients[client_index].fd)) {
                server->sendMessage(clients[client_index].fd, "473 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+i)");
            } else if (channel.hasKey() && key != channel.getKey()) {
                server->sendMessage(clients[client_index].fd, "475 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+k)");
            }
            continue;
        }

        // Add client to channel
        if (channel.addClient(clients[client_index].fd)) {
            clients[client_index].joinChannel(channel_name);
            
            // Send JOIN confirmation to the client
            server->sendMessage(clients[client_index].fd, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " JOIN " + channel_name);
            
            // Broadcast JOIN to other users in the channel
            server->broadcastToChannel(channel_name, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " JOIN " + channel_name, clients[client_index].fd);
            
            // Send topic if exists
            if (!channel.getTopic().empty()) {
                server->sendMessage(clients[client_index].fd, "332 " + clients[client_index].nickname + " " + channel_name + " :" + channel.getTopic());
            }
            
            // Send user list
            server->sendChannelUserList(client_index, channel_name);
            
            std::cout << "Client " << clients[client_index].nickname << " joined channel " << channel_name << std::endl;
        }
    }
}

void CommandHandler::handlePart(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " PART :Not enough parameters");
        return;
    }

    std::string channel_names = msg.params[0];
    std::string part_message = msg.trailing.empty() ? clients[client_index].nickname : msg.trailing;

    std::istringstream channels_stream(channel_names);
    std::string channel_name;
    
    while (std::getline(channels_stream, channel_name, ',')) {
        if (channels.find(channel_name) == channels.end() || !channels[channel_name].hasClient(clients[client_index].fd)) {
            server->sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
            continue;
        }

        // Remove client from channel
        channels[channel_name].removeClient(clients[client_index].fd);
        clients[client_index].leaveChannel(channel_name);
        
        // Send PART message to channel members (including the leaving client)
        std::string part_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PART " + channel_name + " :" + part_message;
        server->broadcastToChannel(channel_name, part_msg);
        server->sendMessage(clients[client_index].fd, part_msg);
        
        std::cout << "Client " << clients[client_index].nickname << " left channel " << channel_name << std::endl;
    }
    
    server->cleanupEmptyChannels();
}

void CommandHandler::handlePrivmsg(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty() || msg.trailing.empty()) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " PRIVMSG :Not enough parameters");
        return;
    }

    std::string target = msg.params[0];
    std::string message = msg.trailing;
    
    // Check if target is a channel
    if (target[0] == '#' || target[0] == '&') {
        // Channel message
        if (channels.find(target) == channels.end()) {
            server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + target + " :No such channel");
            return;
        }
        
        if (!channels[target].hasClient(clients[client_index].fd)) {
            server->sendMessage(clients[client_index].fd, "404 " + clients[client_index].nickname + " " + target + " :Cannot send to channel");
            return;
        }
        
        // Broadcast to channel (excluding sender)
        std::string full_message = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PRIVMSG " + target + " :" + message;
        server->broadcastToChannel(target, full_message, clients[client_index].fd);
    } else {
        // Private message to user
        int target_client = server->findClientByNickname(target);
        if (target_client == -1) {
            server->sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target + " :No such nick");
            return;
        }
        
        // Send private message
        std::string full_message = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PRIVMSG " + target + " :" + message;
        server->sendMessage(clients[target_client].fd, full_message);
    }
}

void CommandHandler::handleKick(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.size() < 2) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " KICK :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];
    std::string target_nick = msg.params[1];
    std::string kick_reason = msg.trailing.empty() ? clients[client_index].nickname : msg.trailing;

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if client is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // Check if client is an operator
    if (!channel.isOperator(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Find target client
    int target_index = server->findClientByNickname(target_nick);
    if (target_index == -1) {
        server->sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        return;
    }

    // Check if target is in the channel
    if (!channel.hasClient(clients[target_index].fd)) {
        server->sendMessage(clients[client_index].fd, "441 " + clients[client_index].nickname + " " + target_nick + " " + channel_name + " :They aren't on that channel");
        return;
    }

    // Perform the kick
    channel.removeClient(clients[target_index].fd);
    clients[target_index].leaveChannel(channel_name);

    // Send KICK message to channel (including the kicked user)
    std::string kick_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " KICK " + channel_name + " " + target_nick + " :" + kick_reason;
    server->broadcastToChannel(channel_name, kick_msg);
    server->sendMessage(clients[target_index].fd, kick_msg);

    std::cout << "Client " << clients[client_index].nickname << " kicked " << target_nick << " from " << channel_name << std::endl;
    server->cleanupEmptyChannels();
}

void CommandHandler::handleInvite(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.size() < 2) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " INVITE :Not enough parameters");
        return;
    }

    std::string target_nick = msg.params[0];
    std::string channel_name = msg.params[1];

    // Find target client
    int target_index = server->findClientByNickname(target_nick);
    if (target_index == -1) {
        server->sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        return;
    }

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if inviter is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // Check if inviter is an operator (required for invite-only channels)
    if (channel.isInviteOnly() && !channel.isOperator(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Check if target is already in the channel
    if (channel.hasClient(clients[target_index].fd)) {
        server->sendMessage(clients[client_index].fd, "443 " + clients[client_index].nickname + " " + target_nick + " " + channel_name + " :is already on channel");
        return;
    }

    // Add to invite list
    channel.inviteClient(clients[target_index].fd);

    // Send invite confirmation to inviter
    server->sendMessage(clients[client_index].fd, "341 " + clients[client_index].nickname + " " + target_nick + " " + channel_name);

    // Send invite notification to target
    server->sendMessage(clients[target_index].fd, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " INVITE " + target_nick + " " + channel_name);

    std::cout << "Client " << clients[client_index].nickname << " invited " << target_nick << " to " << channel_name << std::endl;
}

void CommandHandler::handleTopic(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " TOPIC :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if client is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // If no new topic provided, show current topic
    if (msg.trailing.empty() && msg.params.size() == 1) {
        if (channel.getTopic().empty()) {
            server->sendMessage(clients[client_index].fd, "331 " + clients[client_index].nickname + " " + channel_name + " :No topic is set");
        } else {
            server->sendMessage(clients[client_index].fd, "332 " + clients[client_index].nickname + " " + channel_name + " :" + channel.getTopic());
        }
        return;
    }

    // Check if topic is restricted to operators
    if (channel.isTopicRestricted() && !channel.isOperator(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Set new topic
    std::string new_topic = msg.trailing;
    channel.setTopic(new_topic);

    // Broadcast topic change to channel
    std::string topic_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " TOPIC " + channel_name + " :" + new_topic;
    server->broadcastToChannel(channel_name, topic_msg);

    std::cout << "Client " << clients[client_index].nickname << " changed topic of " << channel_name << " to: " << new_topic << std::endl;
}

void CommandHandler::handleMode(int client_index, const IRCMessage& msg) {
    std::vector<Client>& clients = server->getClients();
    std::map<std::string, Channel>& channels = server->getChannels();
    
    if (!clients[client_index].isFullyRegistered()) {
        server->sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        server->sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // If no mode string provided, show current modes
    if (msg.params.size() == 1) {
        std::string mode_string = channel.getModeString();
        if (mode_string.empty()) {
            server->sendMessage(clients[client_index].fd, "324 " + clients[client_index].nickname + " " + channel_name + " +");
        } else {
            server->sendMessage(clients[client_index].fd, "324 " + clients[client_index].nickname + " " + channel_name + " " + mode_string);
        }
        return;
    }

    // Check if client is an operator
    if (!channel.isOperator(clients[client_index].fd)) {
        server->sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    std::string mode_string = msg.params[1];
    bool adding = true;
    int param_index = 2;

    for (size_t i = 0; i < mode_string.length(); i++) {
        char mode = mode_string[i];
        
        if (mode == '+') {
            adding = true;
        } else if (mode == '-') {
            adding = false;
        } else {
            std::string param = "";
            if (param_index < static_cast<int>(msg.params.size())) {
                param = msg.params[param_index];
            }
            
            switch (mode) {
                case 'i': // Invite-only
                    channel.setInviteOnly(adding);
                    break;
                case 't': // Topic restricted
                    channel.setTopicRestricted(adding);
                    break;
                case 'k': // Channel key
                    if (adding) {
                        if (param.empty()) {
                            server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
                            continue;
                        }
                        channel.setKey(param);
                        param_index++;
                    } else {
                        channel.removeKey();
                    }
                    break;
                case 'l': // User limit
                    if (adding) {
                        if (param.empty()) {
                            server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
                            continue;
                        }
                        long limit = strtol(param.c_str(), NULL, 10);
                        if (limit > 0) {
                            channel.setUserLimit(static_cast<size_t>(limit));
                        }
                        param_index++;
                    } else {
                        channel.removeUserLimit();
                    }
                    break;
                case 'o': // Operator privilege
                    if (param.empty()) {
                        server->sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
                        continue;
                    }
                    int target_index = server->findClientByNickname(param);
                    if (target_index == -1) {
                        server->sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + param + " :No such nick");
                    } else if (!channel.hasClient(clients[target_index].fd)) {
                        server->sendMessage(clients[client_index].fd, "441 " + clients[client_index].nickname + " " + param + " " + channel_name + " :They aren't on that channel");
                    } else {
                        if (adding) {
                            channel.addOperator(clients[target_index].fd);
                        } else {
                            channel.removeOperator(clients[target_index].fd);
                        }
                    }
                    param_index++;
                    break;
            }
        }
    }

    // Broadcast mode change to channel
    std::string mode_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " MODE " + channel_name + " " + mode_string;
    for (int i = 2; i < param_index && i < static_cast<int>(msg.params.size()); i++) {
        mode_msg += " " + msg.params[i];
    }
    server->broadcastToChannel(channel_name, mode_msg);

    std::cout << "Client " << clients[client_index].nickname << " changed mode of " << channel_name << ": " << mode_string << std::endl;
}