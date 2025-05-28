#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include <string>
#include "IRCMessage.hpp"

class Server; // Forward declaration

class CommandHandler {
private:
    Server* server; // Reference to the server instance

public:
    CommandHandler(Server* srv);
    ~CommandHandler();

    // IRC command handlers
    void handleIRCMessage(int client_index, const IRCMessage& msg);
    void handlePass(int client_index, const IRCMessage& msg);
    void handleNick(int client_index, const IRCMessage& msg);
    void handleUser(int client_index, const IRCMessage& msg);
    void handlePing(int client_index, const IRCMessage& msg);
    void handleQuit(int client_index, const IRCMessage& msg);
    void handleWhois(int client_index, const IRCMessage& msg);
    
    // Channel-related command handlers
    void handleJoin(int client_index, const IRCMessage& msg);
    void handlePart(int client_index, const IRCMessage& msg);
    void handlePrivmsg(int client_index, const IRCMessage& msg);
    void handleKick(int client_index, const IRCMessage& msg);
    void handleInvite(int client_index, const IRCMessage& msg);
    void handleTopic(int client_index, const IRCMessage& msg);
    void handleMode(int client_index, const IRCMessage& msg);
};

#endif