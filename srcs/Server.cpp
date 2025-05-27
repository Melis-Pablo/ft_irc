#include "Server.hpp"

Server::Server(const std::string& port_str, const std::string& pass) : server_fd(-1), password(pass) {
    // Parse port
    char *end;
    long temp = strtol(port_str.c_str(), &end, 10);
    if (temp < 1024 || temp > 65535 || *end != '\0')
    {
        throw std::runtime_error("Invalid Port");
    }
    port = static_cast<int>(temp);
    std::cout << "Port parsed: " << port << std::endl;

    setupSocket();
}

Server::~Server() {
    // Clean up all client connections
    for (size_t i = 0; i < clients.size(); i++)
    {
        close(clients[i].fd);
    }
    
    // Close server socket
    if (server_fd >= 0)
    {
        close(server_fd);
    }
}

void Server::setupSocket() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        throw std::runtime_error("socket creation failed");
    }

    // Set socket to non-blocking mode - CRITICAL FIX
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(server_fd);
        throw std::runtime_error("fcntl failed to set non-blocking");
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("setsockopt failed");
    }

    // Setup server address
    struct sockaddr_in server_add;
    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = INADDR_ANY;
    server_add.sin_port = htons(port);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_add, sizeof(server_add)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("bind failed");
    }

    // Start listening
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }

    std::cout << "Server listening on port " << port << std::endl;
}

void Server::acceptNewClient() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd < 0)
    {
        // In non-blocking mode, EAGAIN/EWOULDBLOCK is normal when no connection is pending
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }

    // Set client socket to non-blocking mode - CRITICAL FIX
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl failed for client socket");
        close(client_fd);
        return;
    }

    if (clients.size() >= MAX_CLIENTS)
    {
        std::cerr << "Maximum amount of Clients reached. Connection rejected :(" << std::endl;
        close(client_fd);
    }
    else
    {
        Client new_client;
        new_client.fd = client_fd;
        new_client.hostname = inet_ntoa(client_addr.sin_addr);
        clients.push_back(new_client);
        std::cout << "New client connected. client_fd: " << new_client.fd 
                  << " from " << new_client.hostname << std::endl;
    }
}

void Server::handleClientMessage(int client_index) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_recv = recv(clients[client_index].fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_recv < 0)
    {
        // In non-blocking mode, EAGAIN/EWOULDBLOCK means no data available right now
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return; // This is normal, just return and try again later
        
        // Any other error means connection problem
        perror("recv");
        std::cout << "Client " << clients[client_index].fd << " disconnected due to error" << std::endl;
        removeClient(client_index);
        return;
    }
    
    if (bytes_recv == 0)
    {
        std::cout << "Client " << clients[client_index].fd << " disconnected" << std::endl;
        removeClient(client_index);
        return;
    }

    buffer[bytes_recv] = '\0';
    clients[client_index].buffer += buffer;

    // Process complete messages (ending with \r\n or \n)
    size_t pos = 0;
    while ((pos = clients[client_index].buffer.find('\n')) != std::string::npos)
    {
        std::string message = clients[client_index].buffer.substr(0, pos);
        clients[client_index].buffer.erase(0, pos + 1);
        
        if (!message.empty())
        {
            std::cout << "Client " << clients[client_index].fd << " sent: " << message << std::endl;
            IRCMessage parsed_msg = parseMessage(message);
            handleIRCMessage(client_index, parsed_msg);
        }
    }
}

// Implement the handler:
void Server::handleWhois(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "431 " + clients[client_index].nickname + " :No nickname given");
        return;
    }

    std::string target_nick = msg.params[0];
    int target_index = findClientByNickname(target_nick);
    
    if (target_index == -1) {
        sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        sendMessage(clients[client_index].fd, "318 " + clients[client_index].nickname + " " + target_nick + " :End of WHOIS list");
        return;
    }

    const Client& target = clients[target_index];
    
    // Send WHOIS information
    sendMessage(clients[client_index].fd, "311 " + clients[client_index].nickname + " " + target.nickname + " " + target.username + " " + target.hostname + " * :" + target.realname);
    sendMessage(clients[client_index].fd, "318 " + clients[client_index].nickname + " " + target.nickname + " :End of WHOIS list");
}

void Server::handleIRCMessage(int client_index, const IRCMessage& msg) {
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
        if (clients[client_index].isFullyRegistered()) {
            sendMessage(clients[client_index].fd, "421 " + clients[client_index].nickname + " " + cmd + " :Unknown command");
        }
    }
}

void Server::handlePass(int client_index, const IRCMessage& msg) {
    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "461 * PASS :Not enough parameters");
        return;
    }

    if (clients[client_index].authenticated) {
        sendMessage(clients[client_index].fd, "462 * :You may not reregister");
        return;
    }

    if (msg.params[0] == password) {
        clients[client_index].authenticated = true;
        std::cout << "Client " << clients[client_index].fd << " authenticated successfully" << std::endl;
    } else {
        sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        std::cout << "Client " << clients[client_index].fd << " failed authentication" << std::endl;
    }
}

void Server::handleNick(int client_index, const IRCMessage& msg) {
    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "431 * :No nickname given");
        return;
    }

    // Check if client provided password first
    if (!clients[client_index].authenticated) {
        sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        return;
    }

    std::string new_nick = msg.params[0];
    
    // Check if nickname is already in use
    if (isNicknameInUse(new_nick, client_index)) {
        sendMessage(clients[client_index].fd, "433 * " + new_nick + " :Nickname is already in use");
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
        sendWelcomeMessages(client_index);
    }
}

void Server::handleUser(int client_index, const IRCMessage& msg) {
    if (msg.params.size() < 3 || msg.trailing.empty()) {
        sendMessage(clients[client_index].fd, "461 * USER :Not enough parameters");
        return;
    }

    // Check if client provided password first
    if (!clients[client_index].authenticated) {
        sendMessage(clients[client_index].fd, "464 * :Password incorrect");
        return;
    }

    if (!clients[client_index].username.empty()) {
        sendMessage(clients[client_index].fd, "462 * :You may not reregister");
        return;
    }

    clients[client_index].username = msg.params[0];
    clients[client_index].realname = msg.trailing;
    
    std::cout << "Client " << clients[client_index].fd << " set username to: " << clients[client_index].username 
              << " realname: " << clients[client_index].realname << std::endl;

    // If client is now fully registered, send welcome messages
    if (clients[client_index].isFullyRegistered() && !clients[client_index].registered) {
        clients[client_index].registered = true;
        sendWelcomeMessages(client_index);
    }
}

void Server::handlePing(int client_index, const IRCMessage& msg) {
    std::string response = "PONG";
    if (!msg.params.empty()) {
        response += " :" + msg.params[0];
    } else if (!msg.trailing.empty()) {
        response += " :" + msg.trailing;
    }
    sendMessage(clients[client_index].fd, response);
}

void Server::handleQuit(int client_index, const IRCMessage& msg) {
    std::string quit_msg = msg.trailing.empty() ? "Client Quit" : msg.trailing;
    std::cout << "Client " << clients[client_index].fd << " (" << clients[client_index].nickname 
              << ") quit: " << quit_msg << std::endl;
    removeClient(client_index);
}

void Server::handleJoin(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " JOIN :Not enough parameters");
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
        
        if (!isValidChannelName(channel_name)) {
            sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
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
                sendMessage(clients[client_index].fd, "471 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+l)");
            } else if (channel.isInviteOnly() && !channel.isInvited(clients[client_index].fd)) {
                sendMessage(clients[client_index].fd, "473 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+i)");
            } else if (channel.hasKey() && key != channel.getKey()) {
                sendMessage(clients[client_index].fd, "475 " + clients[client_index].nickname + " " + channel_name + " :Cannot join channel (+k)");
            }
            continue;
        }

        // Add client to channel
        if (channel.addClient(clients[client_index].fd)) {
            clients[client_index].joinChannel(channel_name);
            
            // Send JOIN confirmation to the client
            sendMessage(clients[client_index].fd, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " JOIN " + channel_name);
            
            // Broadcast JOIN to other users in the channel
            broadcastToChannel(channel_name, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " JOIN " + channel_name, clients[client_index].fd);
            
            // Send topic if exists
            if (!channel.getTopic().empty()) {
                sendMessage(clients[client_index].fd, "332 " + clients[client_index].nickname + " " + channel_name + " :" + channel.getTopic());
            }
            
            // Send user list
            sendChannelUserList(client_index, channel_name);
            
            std::cout << "Client " << clients[client_index].nickname << " joined channel " << channel_name << std::endl;
        }
    }
}

void Server::handlePart(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " PART :Not enough parameters");
        return;
    }

    std::string channel_names = msg.params[0];
    std::string part_message = msg.trailing.empty() ? clients[client_index].nickname : msg.trailing;

    std::istringstream channels_stream(channel_names);
    std::string channel_name;
    
    while (std::getline(channels_stream, channel_name, ',')) {
        if (channels.find(channel_name) == channels.end() || !channels[channel_name].hasClient(clients[client_index].fd)) {
            sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
            continue;
        }

        // Remove client from channel
        channels[channel_name].removeClient(clients[client_index].fd);
        clients[client_index].leaveChannel(channel_name);
        
        // Send PART message to channel members (including the leaving client)
        std::string part_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PART " + channel_name + " :" + part_message;
        broadcastToChannel(channel_name, part_msg);
        sendMessage(clients[client_index].fd, part_msg);
        
        std::cout << "Client " << clients[client_index].nickname << " left channel " << channel_name << std::endl;
    }
    
    cleanupEmptyChannels();
}

void Server::handlePrivmsg(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty() || msg.trailing.empty()) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " PRIVMSG :Not enough parameters");
        return;
    }

    std::string target = msg.params[0];
    std::string message = msg.trailing;
    
    // Check if target is a channel
    if (target[0] == '#' || target[0] == '&') {
        // Channel message
        if (channels.find(target) == channels.end()) {
            sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + target + " :No such channel");
            return;
        }
        
        if (!channels[target].hasClient(clients[client_index].fd)) {
            sendMessage(clients[client_index].fd, "404 " + clients[client_index].nickname + " " + target + " :Cannot send to channel");
            return;
        }
        
        // Broadcast to channel (excluding sender)
        std::string full_message = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PRIVMSG " + target + " :" + message;
        broadcastToChannel(target, full_message, clients[client_index].fd);
    } else {
        // Private message to user
        int target_client = findClientByNickname(target);
        if (target_client == -1) {
            sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target + " :No such nick");
            return;
        }
        
        // Send private message
        std::string full_message = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " PRIVMSG " + target + " :" + message;
        sendMessage(clients[target_client].fd, full_message);
    }
}

void Server::handleKick(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.size() < 2) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " KICK :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];
    std::string target_nick = msg.params[1];
    std::string kick_reason = msg.trailing.empty() ? clients[client_index].nickname : msg.trailing;

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if client is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // Check if client is an operator
    if (!channel.isOperator(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Find target client
    int target_index = findClientByNickname(target_nick);
    if (target_index == -1) {
        sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        return;
    }

    // Check if target is in the channel
    if (!channel.hasClient(clients[target_index].fd)) {
        sendMessage(clients[client_index].fd, "441 " + clients[client_index].nickname + " " + target_nick + " " + channel_name + " :They aren't on that channel");
        return;
    }

    // Perform the kick
    channel.removeClient(clients[target_index].fd);
    clients[target_index].leaveChannel(channel_name);

    // Send KICK message to channel (including the kicked user)
    std::string kick_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " KICK " + channel_name + " " + target_nick + " :" + kick_reason;
    broadcastToChannel(channel_name, kick_msg);
    sendMessage(clients[target_index].fd, kick_msg);

    std::cout << "Client " << clients[client_index].nickname << " kicked " << target_nick << " from " << channel_name << std::endl;
    cleanupEmptyChannels();
}

void Server::handleInvite(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.size() < 2) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " INVITE :Not enough parameters");
        return;
    }

    std::string target_nick = msg.params[0];
    std::string channel_name = msg.params[1];

    // Find target client
    int target_index = findClientByNickname(target_nick);
    if (target_index == -1) {
        sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + target_nick + " :No such nick");
        return;
    }

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if inviter is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // Check if inviter is an operator (required for invite-only channels)
    if (channel.isInviteOnly() && !channel.isOperator(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Check if target is already in the channel
    if (channel.hasClient(clients[target_index].fd)) {
        sendMessage(clients[client_index].fd, "443 " + clients[client_index].nickname + " " + target_nick + " " + channel_name + " :is already on channel");
        return;
    }

    // Add to invite list
    channel.inviteClient(clients[target_index].fd);

    // Send invite confirmation to inviter
    sendMessage(clients[client_index].fd, "341 " + clients[client_index].nickname + " " + target_nick + " " + channel_name);

    // Send invite notification to target
    sendMessage(clients[target_index].fd, ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " INVITE " + target_nick + " " + channel_name);

    std::cout << "Client " << clients[client_index].nickname << " invited " << target_nick << " to " << channel_name << std::endl;
}

void Server::handleTopic(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " TOPIC :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // Check if client is in the channel
    if (!channel.hasClient(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "442 " + clients[client_index].nickname + " " + channel_name + " :You're not on that channel");
        return;
    }

    // If no new topic provided, show current topic
    if (msg.trailing.empty() && msg.params.size() == 1) {
        if (channel.getTopic().empty()) {
            sendMessage(clients[client_index].fd, "331 " + clients[client_index].nickname + " " + channel_name + " :No topic is set");
        } else {
            sendMessage(clients[client_index].fd, "332 " + clients[client_index].nickname + " " + channel_name + " :" + channel.getTopic());
        }
        return;
    }

    // Check if topic is restricted to operators
    if (channel.isTopicRestricted() && !channel.isOperator(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
        return;
    }

    // Set new topic
    std::string new_topic = msg.trailing;
    channel.setTopic(new_topic);

    // Broadcast topic change to channel
    std::string topic_msg = ":" + clients[client_index].nickname + "!" + clients[client_index].username + "@" + clients[client_index].hostname + " TOPIC " + channel_name + " :" + new_topic;
    broadcastToChannel(channel_name, topic_msg);

    std::cout << "Client " << clients[client_index].nickname << " changed topic of " << channel_name << " to: " << new_topic << std::endl;
}

void Server::handleMode(int client_index, const IRCMessage& msg) {
    if (!clients[client_index].isFullyRegistered()) {
        sendMessage(clients[client_index].fd, "451 * :You have not registered");
        return;
    }

    if (msg.params.empty()) {
        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
        return;
    }

    std::string channel_name = msg.params[0];

    // Check if channel exists
    if (channels.find(channel_name) == channels.end()) {
        sendMessage(clients[client_index].fd, "403 " + clients[client_index].nickname + " " + channel_name + " :No such channel");
        return;
    }

    Channel& channel = channels[channel_name];

    // If no mode string provided, show current modes
    if (msg.params.size() == 1) {
        std::string mode_string = channel.getModeString();
        if (mode_string.empty()) {
            sendMessage(clients[client_index].fd, "324 " + clients[client_index].nickname + " " + channel_name + " +");
        } else {
            sendMessage(clients[client_index].fd, "324 " + clients[client_index].nickname + " " + channel_name + " " + mode_string);
        }
        return;
    }

    // Check if client is an operator
    if (!channel.isOperator(clients[client_index].fd)) {
        sendMessage(clients[client_index].fd, "482 " + clients[client_index].nickname + " " + channel_name + " :You're not channel operator");
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
                            sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
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
                            sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
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
                        sendMessage(clients[client_index].fd, "461 " + clients[client_index].nickname + " MODE :Not enough parameters");
                        continue;
                    }
                    int target_index = findClientByNickname(param);
                    if (target_index == -1) {
                        sendMessage(clients[client_index].fd, "401 " + clients[client_index].nickname + " " + param + " :No such nick");
                    } else if (!channel.hasClient(clients[target_index].fd)) {
                        sendMessage(clients[client_index].fd, "441 " + clients[client_index].nickname + " " + param + " " + channel_name + " :They aren't on that channel");
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
    broadcastToChannel(channel_name, mode_msg);

    std::cout << "Client " << clients[client_index].nickname << " changed mode of " << channel_name << ": " << mode_string << std::endl;
}

void Server::sendMessage(int client_fd, const std::string& message) {
    std::string full_message = message + "\r\n";
    ssize_t bytes_sent = send(client_fd, full_message.c_str(), full_message.length(), 0);
    if (bytes_sent < 0) {
        perror("send");
    }
    // Add flush to ensure immediate sending
    fsync(client_fd);
}

void Server::sendWelcomeMessages(int client_index) {
    const Client& client = clients[client_index];
    std::string nick = client.nickname;
    
    sendMessage(client.fd, "001 " + nick + " :Welcome to the IRC Server, " + nick + "!");
    sendMessage(client.fd, "002 " + nick + " :Your host is ircserv, running version 1.0");
    sendMessage(client.fd, "003 " + nick + " :This server was created today");
    sendMessage(client.fd, "004 " + nick + " ircserv 1.0 o o");
    
    std::cout << "Sent welcome messages to " << nick << std::endl;
}

bool Server::isNicknameInUse(const std::string& nickname, int exclude_client_index) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (static_cast<int>(i) != exclude_client_index && clients[i].nickname == nickname) {
            return true;
        }
    }
    return false;
}

bool Server::isValidChannelName(const std::string& name) {
    return !name.empty() && (name[0] == '#' || name[0] == '&') && name.length() > 1;
}

void Server::broadcastToChannel(const std::string& channel_name, const std::string& message, int exclude_client_fd) {
    if (channels.find(channel_name) == channels.end()) {
        return;
    }

    const std::set<int>& channel_clients = channels[channel_name].getClients();
    for (std::set<int>::const_iterator it = channel_clients.begin(); it != channel_clients.end(); ++it) {
        if (*it != exclude_client_fd) {
            sendMessage(*it, message);
        }
    }
}

void Server::sendChannelUserList(int client_index, const std::string& channel_name) {
    if (channels.find(channel_name) == channels.end()) {
        return;
    }

    const Channel& channel = channels[channel_name];
    const std::set<int>& channel_clients = channel.getClients();
    
    std::string user_list = "";
    for (std::set<int>::const_iterator it = channel_clients.begin(); it != channel_clients.end(); ++it) {
        // Find client by fd
        for (size_t i = 0; i < clients.size(); i++) {
            if (clients[i].fd == *it) {
                if (!user_list.empty()) user_list += " ";
                if (channel.isOperator(*it)) user_list += "@";
                user_list += clients[i].nickname;
                break;
            }
        }
    }

    sendMessage(clients[client_index].fd, "353 " + clients[client_index].nickname + " = " + channel_name + " :" + user_list);
    sendMessage(clients[client_index].fd, "366 " + clients[client_index].nickname + " " + channel_name + " :End of NAMES list");
}

int Server::findClientByNickname(const std::string& nickname) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i].nickname == nickname) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Server::removeClientFromAllChannels(int client_index) {
    const std::set<std::string>& client_channels = clients[client_index].getChannels();
    for (std::set<std::string>::const_iterator it = client_channels.begin(); it != client_channels.end(); ++it) {
        if (channels.find(*it) != channels.end()) {
            channels[*it].removeClient(clients[client_index].fd);
        }
    }
}

void Server::cleanupEmptyChannels() {
    std::map<std::string, Channel>::iterator it = channels.begin();
    while (it != channels.end()) {
        if (it->second.isEmpty()) {
            std::map<std::string, Channel>::iterator to_erase = it;
            ++it;
            channels.erase(to_erase);
        } else {
            ++it;
        }
    }
}

void Server::removeClient(int client_index) {
    removeClientFromAllChannels(client_index);
    close(clients[client_index].fd);
    clients.erase(clients.begin() + client_index);
    cleanupEmptyChannels();
}

void Server::run() {
    std::vector<struct pollfd> poll_fds;
    
    while (true) {
        poll_fds.clear();
        
        // Add server socket
        struct pollfd server_pollfd;
        server_pollfd.fd = server_fd;
        server_pollfd.events = POLLIN;
        server_pollfd.revents = 0;
        poll_fds.push_back(server_pollfd);
        
        // Add client sockets
        for (size_t i = 0; i < clients.size(); i++) {
            struct pollfd client_pollfd;
            client_pollfd.fd = clients[i].fd;
            client_pollfd.events = POLLIN;
            client_pollfd.revents = 0;
            poll_fds.push_back(client_pollfd);
        }
        
        // Poll for events
        int poll_result = poll(&poll_fds[0], poll_fds.size(), -1);
        if (poll_result < 0) {
            perror("poll");
            break;
        }
        
        // Check server socket for new connections
        if (poll_fds[0].revents & POLLIN) {
            acceptNewClient();
        }
        
        // Check client sockets for messages
        for (size_t i = 1; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLIN) {
                // Find client index by fd
                for (int j = static_cast<int>(clients.size()) - 1; j >= 0; j--) {
                    if (clients[j].fd == poll_fds[i].fd) {
                        handleClientMessage(j);
                        break;
                    }
                }
            }
        }
    }
}