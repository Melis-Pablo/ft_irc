#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>

class Client;

class Channel {
    private:
        std::string name;
        std::string topic;
        std::string key;           // Channel password (mode +k)
        std::set<int> clients;     // Client file descriptors in this channel
        std::set<int> operators;   // Operator client file descriptors
        bool invite_only;          // Mode +i
        bool topic_restricted;     // Mode +t (only operators can change topic)
        bool has_key;              // Mode +k
        bool has_user_limit;       // Mode +l
        size_t user_limit;         // Maximum users allowed
        std::set<int> invited_clients;  // Clients invited to invite-only channel

    public:
        Channel(); // Default constructor for std::map
        Channel(const std::string& channel_name);
        ~Channel();

        // Getters
        const std::string& getName() const { return name; }
        const std::string& getTopic() const { return topic; }
        const std::string& getKey() const { return key; }
        bool isInviteOnly() const { return invite_only; }
        bool isTopicRestricted() const { return topic_restricted; }
        bool hasKey() const { return has_key; }
        bool hasUserLimit() const { return has_user_limit; }
        size_t getUserLimit() const { return user_limit; }
        size_t getUserCount() const { return clients.size(); }
        
        // Client management
        bool hasClient(int client_fd) const;
        bool addClient(int client_fd);
        bool removeClient(int client_fd);
        const std::set<int>& getClients() const { return clients; }
        
        // Operator management
        bool isOperator(int client_fd) const;
        void addOperator(int client_fd);
        void removeOperator(int client_fd);
        const std::set<int>& getOperators() const { return operators; }
        
        // Channel modes
        void setTopic(const std::string& new_topic);
        void setInviteOnly(bool invite_only);
        void setTopicRestricted(bool restricted);
        void setKey(const std::string& new_key);
        void removeKey();
        void setUserLimit(size_t limit);
        void removeUserLimit();
        
        // Invite management
        void inviteClient(int client_fd);
        bool isInvited(int client_fd) const;
        void removeInvite(int client_fd);
        
        // Utility
        bool canJoin(int client_fd, const std::string& provided_key = "") const;
        std::string getModeString() const;
        bool isEmpty() const { return clients.empty(); }
};

#endif