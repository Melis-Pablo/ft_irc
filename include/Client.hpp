#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
    public:
        int fd;
        std::string nickname;
        std::string username;
        std::string realname;
        std::string hostname;
        bool authenticated;
        bool registered;
        std::string buffer;

        Client();
        ~Client();

        bool isFullyRegistered() const;
};

#endif