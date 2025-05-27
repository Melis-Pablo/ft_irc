#include "Channel.hpp"

Channel::Channel() : name(""), topic(""), key(""), invite_only(false), topic_restricted(false), has_key(false), has_user_limit(false), user_limit(0) {
}

Channel::Channel(const std::string& channel_name) : name(channel_name), topic(""), key(""), invite_only(false), topic_restricted(false), has_key(false), has_user_limit(false), user_limit(0) {
}

Channel::~Channel() {
}

bool Channel::hasClient(int client_fd) const {
    return clients.find(client_fd) != clients.end();
}

bool Channel::addClient(int client_fd) {
    if (hasClient(client_fd)) {
        return false; // Already in channel
    }
    
    if (has_user_limit && clients.size() >= user_limit) {
        return false; // Channel is full
    }
    
    clients.insert(client_fd);
    
    // Make first client an operator
    if (clients.size() == 1) {
        operators.insert(client_fd);
    }
    
    // Remove from invited list if they were invited
    removeInvite(client_fd);
    
    return true;
}

bool Channel::removeClient(int client_fd) {
    if (!hasClient(client_fd)) {
        return false;
    }
    
    clients.erase(client_fd);
    operators.erase(client_fd); // Remove operator status if they had it
    removeInvite(client_fd);    // Remove any pending invite
    
    return true;
}

bool Channel::isOperator(int client_fd) const {
    return operators.find(client_fd) != operators.end();
}

void Channel::addOperator(int client_fd) {
    if (hasClient(client_fd)) {
        operators.insert(client_fd);
    }
}

void Channel::removeOperator(int client_fd) {
    operators.erase(client_fd);
}

void Channel::setTopic(const std::string& new_topic) {
    topic = new_topic;
}

void Channel::setInviteOnly(bool invite_only_mode) {
    invite_only = invite_only_mode;
}

void Channel::setTopicRestricted(bool restricted) {
    topic_restricted = restricted;
}

void Channel::setKey(const std::string& new_key) {
    key = new_key;
    has_key = true;
}

void Channel::removeKey() {
    key.clear();
    has_key = false;
}

void Channel::setUserLimit(size_t limit) {
    user_limit = limit;
    has_user_limit = true;
}

void Channel::removeUserLimit() {
    user_limit = 0;
    has_user_limit = false;
}

void Channel::inviteClient(int client_fd) {
    invited_clients.insert(client_fd);
}

bool Channel::isInvited(int client_fd) const {
    return invited_clients.find(client_fd) != invited_clients.end();
}

void Channel::removeInvite(int client_fd) {
    invited_clients.erase(client_fd);
}

bool Channel::canJoin(int client_fd, const std::string& provided_key) const {
    // Check if channel has user limit and is full
    if (has_user_limit && clients.size() >= user_limit) {
        return false;
    }
    
    // Check if channel is invite-only
    if (invite_only && !isInvited(client_fd)) {
        return false;
    }
    
    // Check if channel has a key
    if (has_key && provided_key != key) {
        return false;
    }
    
    return true;
}

std::string Channel::getModeString() const {
    std::string modes = "+";
    std::string params = "";
    
    if (invite_only) modes += "i";
    if (topic_restricted) modes += "t";
    if (has_key) {
        modes += "k";
        if (!params.empty()) params += " ";
        params += key;
    }
    if (has_user_limit) {
        modes += "l";
        if (!params.empty()) params += " ";
        // Convert user_limit to string
        std::ostringstream oss;
        oss << user_limit;
        params += oss.str();
    }
    
    if (modes == "+") {
        return "";
    }
    
    if (!params.empty()) {
        modes += " " + params;
    }
    
    return modes;
}