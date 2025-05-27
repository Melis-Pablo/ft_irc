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
        perror("accept");
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

    if (bytes_recv <= 0)
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

void Server::sendMessage(int client_fd, const std::string& message) {
    std::string full_message = message + "\r\n";
    send(client_fd, full_message.c_str(), full_message.length(), 0);
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

void Server::removeClient(int client_index) {
    close(clients[client_index].fd);
    clients.erase(clients.begin() + client_index);
}

void Server::run() {
    while (true)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        // Add all client file descriptors to the set
        for (size_t j = 0; j < clients.size(); j++)
        {
            FD_SET(clients[j].fd, &read_fds);
            if (clients[j].fd > max_fd)
                max_fd = clients[j].fd;
        }

        // Wait for activity on any of the file descriptors
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            break;
        }

        // Check if there's a new connection
        if (FD_ISSET(server_fd, &read_fds))
        {
            acceptNewClient();
        }

        // Check all clients for incoming messages
        // Note: iterate backwards to safely remove clients during iteration
        for (int i = static_cast<int>(clients.size()) - 1; i >= 0; i--)
        {
            if (FD_ISSET(clients[i].fd, &read_fds))
            {
                handleClientMessage(i);
            }
        }
    }
}