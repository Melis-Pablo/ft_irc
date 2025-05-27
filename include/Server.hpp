#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdexcept>
#include "Client.hpp"
#include "IRCMessage.hpp"

class Server
{
private:
    static const int MAX_CLIENTS = 5;
    static const int BUFFER_SIZE = 1024;
    
    int server_fd;
    int port;
    std::string password;
    std::vector<Client> clients;

    // Private methods
    void setupSocket();
    void acceptNewClient();
    void handleClientMessage(int client_index);
    void removeClient(int client_index);
    
    // IRC command handlers
    void handleIRCMessage(int client_index, const IRCMessage& msg);
    void handlePass(int client_index, const IRCMessage& msg);
    void handleNick(int client_index, const IRCMessage& msg);
    void handleUser(int client_index, const IRCMessage& msg);
    void handlePing(int client_index, const IRCMessage& msg);
    void handleQuit(int client_index, const IRCMessage& msg);
    
    // Utility methods
    void sendMessage(int client_fd, const std::string& message);
    void sendWelcomeMessages(int client_index);
    bool isNicknameInUse(const std::string& nickname, int exclude_client_index = -1);

public:
    Server(const std::string& port_str, const std::string& pass);
    ~Server();
    
    void run();
};

#endif