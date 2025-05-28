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
#include <fcntl.h> // fcntl
#include <poll.h> // poll (or equivalent [select(), kqueue(), or epoll()])
#include "Client.hpp"
#include "IRCMessage.hpp"
#include "Channel.hpp"

class CommandHandler; // Forward declaration

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
    CommandHandler* commandHandler; // Command handler instance

    // Private methods
    void setupSocket();
    void acceptNewClient();
    void handleClientMessage(int client_index);
    void removeClient(int client_index);

public:
    Server(const std::string& port_str, const std::string& pass);
    ~Server();
    
    void run();

    // Public methods for CommandHandler to use
    void sendMessage(int client_fd, const std::string& message);
    void sendWelcomeMessages(int client_index);
    bool isNicknameInUse(const std::string& nickname, int exclude_client_index = -1);
    bool isValidChannelName(const std::string& name);
    void broadcastToChannel(const std::string& channel_name, const std::string& message, int exclude_client_fd = -1);
    void sendChannelUserList(int client_index, const std::string& channel_name);
    int findClientByNickname(const std::string& nickname);
    void removeClientFromAllChannels(int client_index);
    void cleanupEmptyChannels();

    // Getters for CommandHandler
    std::vector<Client>& getClients() { return clients; }
    std::map<std::string, Channel>& getChannels() { return channels; }
    const std::string& getPassword() const { return password; }
};

#endif