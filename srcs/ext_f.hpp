#ifndef EXT_F_HPP
#define EXT_F_HPP

#include <sys/socket.h> // socket, setsockopt, getsockname, bind, connect, listen, accept
#include <unistd.h> // close, lseek
#include <netinet/in.h> // htons, htonl, ntohs, ntohl
#include <arpa/inet.h> // inet_addr, inet_ntoa
#include <netdb.h> // getprotobyname, gethostbyname, getaddrinfo, freeaddrinfo
#include <csignal> // signal, sigaction
#include <sys/stat.h> // fstat
#include <fcntl.h> // fcntl
#include <poll.h> // poll (or equivalent)
#include <sys/types.h> // recv, send

// Also Everything in C++ 98.
#include <iostream> // std::cout, std::cerr
#include <string> // std::string

#endif