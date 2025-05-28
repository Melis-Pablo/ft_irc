#include "Server.hpp"
#include "CommandHandler.hpp"

Server::Server(const std::string& port_str, const std::string& pass) : server_fd(-1), password(pass), commandHandler(nullptr) {
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
    
    // Initialize command handler
    commandHandler = new CommandHandler(this);
}

Server::~Server() {
    // Clean up command handler
    delete commandHandler;
    
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
            
            // Handle QUIT specially since it needs to remove the client
            if (!parsed_msg.command.empty()) {
                std::string cmd = parsed_msg.command;
                for (size_t i = 0; i < cmd.length(); i++) {
                    cmd[i] = std::toupper(cmd[i]);
                }
                
                if (cmd == "QUIT") {
                    std::string quit_msg = parsed_msg.trailing.empty() ? "Client Quit" : parsed_msg.trailing;
                    std::cout << "Client " << clients[client_index].fd << " (" << clients[client_index].nickname 
                              << ") quit: " << quit_msg << std::endl;
                    removeClient(client_index);
                    return; // Important: return immediately after removing client
                }
            }
            
            // Use command handler for other commands
            commandHandler->handleIRCMessage(client_index, parsed_msg);
        }
    }
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