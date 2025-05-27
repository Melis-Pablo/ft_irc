#ifndef ALLOWED_HPP
#define ALLOWED_HPP

#include <sys/socket.h> // socket, setsockopt, getsockname, bind, connect, listen, accept
#include <unistd.h> // close, lseek
#include <netinet/in.h> // htons, htonl, ntohs, ntohl
#include <arpa/inet.h> // inet_addr, inet_ntoa
#include <netdb.h> // getprotobyname, gethostbyname, getaddrinfo, freeaddrinfo
#include <csignal> // signal, sigaction
#include <sys/stat.h> // fstat
#include <fcntl.h> // fcntl
#include <poll.h> // poll (or equivalent [select(), kqueue(), or epoll()])
#include <sys/types.h> // recv, send

// Also Everything in C++ 98.
#include <string> // For std::string
#include <set> // For std::set
#include <vector> // For std::vector
#include <iostream> // For std::cout 
#include <sstream> // For std::istringstream, std::ostringstream
#include <map> // For std::map
#include <cctype> // For std::isdigit
#include <utility> // For std::pair
#include <cstring> // For std::strerror
#include <cstdlib> // For std::atoi
#include <stdexcept> // For std::runtime_error
#include <sys/select.h> // For select()

#include <errno.h> // For errno
#include <arpa/inet.h> // For inet_pton/, sockaddr_in


#endif