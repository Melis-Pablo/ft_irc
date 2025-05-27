#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), username(""), realname(""), hostname(""), authenticated(false), registered(false), buffer("") {};
        
Client::~Client(){};

bool Client::isFullyRegistered() const {
    return authenticated && !nickname.empty() && !username.empty();
};