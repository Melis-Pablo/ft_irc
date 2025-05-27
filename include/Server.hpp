#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector> // For std::vector
#include <map> // For std::map
#include <string> // For std::string
#include <iostream> // For std::cout
#include <cstring> // For std::strerror
#include <cstdlib> // For std::atoi
#include <unistd.h> // For close()
#include <arpa/inet.h> // For inet_pton, sockaddr_in
#include <sys/socket.h> // For socket functions
#include <sys/select.h> // For select()
#include <stdexcept> // For std::runtime_error
#include <sstream> // For std::istringstream
#include <cctype> // For std::isdigit
#include <utility> // For std::pair
#include <errno.h> // For errno
#include "Allowed.hpp"
#include "Client.hpp"
#include "IRCMessage.hpp"
#include "Channel.hpp"

class Server
{
private:
    static const int MAX_CLIENTS = 5;
    static const int BUFFER_SIZE = 1024;
    
    int server_fd;
    int port;
    std::string password;
    std::vector<Client> clients;
    std::map<std::string, Channel> channels; // Channel name -> Channel object

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
    
    // Channel-related command handlers
    void handleJoin(int client_index, const IRCMessage& msg);
    void handlePart(int client_index, const IRCMessage& msg);
    void handlePrivmsg(int client_index, const IRCMessage& msg);
    void handleKick(int client_index, const IRCMessage& msg);
    void handleInvite(int client_index, const IRCMessage& msg);
    void handleTopic(int client_index, const IRCMessage& msg);
    void handleMode(int client_index, const IRCMessage& msg);
    
    // Utility methods
    void sendMessage(int client_fd, const std::string& message);
    void sendWelcomeMessages(int client_index);
    bool isNicknameInUse(const std::string& nickname, int exclude_client_index = -1);
    
    // Channel utility methods
    bool isValidChannelName(const std::string& name);
    void broadcastToChannel(const std::string& channel_name, const std::string& message, int exclude_client_fd = -1);
    void sendChannelUserList(int client_index, const std::string& channel_name);
    int findClientByNickname(const std::string& nickname);
    void removeClientFromAllChannels(int client_index);
    void cleanupEmptyChannels();

    void handleWhois(int client_index, const IRCMessage& msg);

public:
    Server(const std::string& port_str, const std::string& pass);
    ~Server();
    
    void run();
};

#endif