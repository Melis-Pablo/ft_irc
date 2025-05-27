# ft_irc

## 💬 Project Overview

ft_irc is a network programming project focused on implementing a functional Internet Relay Chat (IRC) server from scratch using C++98. The project demonstrates understanding of network protocols, socket programming, and concurrent client handling without using threads or forking.

The server handles multiple clients simultaneously using non-blocking I/O operations and implements core IRC functionality including user authentication, channel management, private messaging, and operator privileges. All communication follows the IRC protocol standards, making it compatible with existing IRC clients.

## 🏗️ Server Architecture

### Core Components
- **Socket Management**: TCP/IP server socket handling multiple client connections
- **Client Handler**: Non-blocking client connection management
- **Channel System**: Multi-user chat rooms with operator privileges
- **Command Parser**: IRC protocol command processing and validation
- **User Authentication**: Nickname, username, and password verification

### Network Architecture
- Single-threaded event-driven architecture using poll() (or equivalent)
- Non-blocking I/O operations for all socket communications
- TCP/IP (v4 or v6) communication protocol
- Efficient handling of partial data reception and transmission

### IRC Features
- **User Management**: Authentication, nicknames, and user states
- **Channel Operations**: Join, leave, and message broadcasting
- **Private Messaging**: Direct user-to-user communication
- **Operator Commands**: Channel moderation and administration
- **Error Handling**: Proper IRC error responses and edge case management

### Supported IRC Commands

#### Basic Commands
- **PASS**: Server password authentication
- **NICK**: Set or change nickname
- **USER**: Set username and real name
- **JOIN**: Join a channel
- **PART**: Leave a channel
- **PRIVMSG**: Send private messages
- **QUIT**: Disconnect from server

#### Operator Commands
- **KICK**: Remove user from channel
- **INVITE**: Invite user to invite-only channel
- **TOPIC**: Set or view channel topic
- **MODE**: Modify channel settings
  - `i`: Invite-only mode
  - `t`: Topic restriction to operators
  - `k`: Channel password/key
  - `o`: Grant/revoke operator status
  - `l`: Set user limit

## 🔧 Technical Implementation

### Project Structure

```
ft_irc/
├── Makefile
├── includes/
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   └── Commands.hpp
├── srcs/
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Channel.cpp
│   ├── Commands.cpp
│   └── Utils.cpp
└── config/
    └── server.conf (optional)
```

### Design Principles

The server follows these architectural principles:
- **Single-Threaded**: No forking or threading for client handling
- **Event-Driven**: poll() or equivalent for efficient I/O multiplexing
- **Non-Blocking I/O**: All socket operations are non-blocking
- **Protocol Compliance**: Follows IRC RFC specifications
- **Memory Safety**: Proper resource management and error handling

### Socket Programming

The server implements low-level socket operations:
- TCP socket creation and binding
- Non-blocking socket configuration
- Client connection acceptance
- Efficient data transmission and reception
- Proper connection cleanup and error handling

### Command Processing

IRC commands are processed through:
- Message parsing and validation
- Command routing and execution
- Response generation and transmission
- Error handling and user feedback

### Channel Management

Channels provide multi-user communication:
- Dynamic channel creation and destruction
- User membership tracking
- Message broadcasting to channel members
- Operator privilege management
- Mode and setting enforcement

## 🚀 Usage

### Prerequisites
- C++ compiler (g++ or clang++)
- Make
- IRC client (HexChat, WeeChat, irssi, or similar)
- Unix-like operating system

### Compilation and Setup

```bash
# Clone the repository
git clone https://github.com/username/ft_irc.git
cd ft_irc

# Compile the server
make

# Start the IRC server
./ircserv <port> <password>

# Example
./ircserv 6667 mypassword
```

### Connecting with IRC Client

```bash
# Using HexChat, WeeChat, or any IRC client
# Server: localhost (or your server IP)
# Port: 6667 (or your chosen port)
# Password: mypassword (your server password)

# Example with nc for testing
nc localhost 6667
PASS mypassword
NICK mynick
USER myuser 0 * :My Real Name
JOIN #general
PRIVMSG #general :Hello, world!
```

### Makefile Commands

| Command       | Description                           |
|---------------|---------------------------------------|
| `make`        | Compile the IRC server                |
| `make all`    | Same as make                          |
| `make clean`  | Remove object files                   |
| `make fclean` | Remove object files and executable    |
| `make re`     | Recompile from scratch                |

## 🛠️ Development Approach

### Network Programming

The project implements low-level network programming concepts:
- Socket creation, binding, and listening
- Client connection management
- Non-blocking I/O operations
- Event-driven architecture with poll()
- Proper error handling and resource cleanup

### Protocol Implementation

IRC protocol compliance includes:
- Message format parsing (prefix, command, parameters)
- Numeric reply codes
- Error message generation
- Client state management
- Channel state synchronization

### Memory Management

Careful attention to resource management:
- Dynamic client and channel allocation
- Proper cleanup on disconnection
- Buffer management for partial messages
- Memory leak prevention

### Testing Strategy

Comprehensive testing approach:
- Multiple IRC client compatibility
- Concurrent connection handling
- Partial data transmission scenarios
- Error condition testing
- Network disruption simulation

## 📝 Learning Outcomes

This project provided extensive experience with:

- **Network Programming**: Understanding TCP/IP sockets and network communication
- **Protocol Implementation**: Following RFC specifications for IRC protocol
- **Concurrent Programming**: Managing multiple clients without threading
- **Event-Driven Architecture**: Using poll() for efficient I/O multiplexing
- **C++ Systems Programming**: Low-level programming with C++98 standard
- **Client-Server Architecture**: Designing scalable server applications
- **Error Handling**: Robust error management in network applications
- **Memory Management**: Proper resource allocation and cleanup
- **Protocol Parsing**: Parsing and validating network messages
- **State Management**: Tracking complex client and channel states

## 🧪 Testing

### Manual Testing

```bash
# Test basic connection
./ircserv 6667 password &
nc localhost 6667

# Test with IRC client
hexchat # or your preferred IRC client
# Connect to localhost:6667 with password

# Test partial data (using ctrl+D)
nc -C localhost 6667
PASS pass^Dword^D
NICK my^Dnick^D
```

### Test Scenarios

- Multiple simultaneous client connections
- Channel operations with multiple users
- Operator privilege enforcement
- Partial message handling
- Client disconnection scenarios
- Invalid command handling
- Network error conditions

## ⚙️ Bonus Features

Additional features that can be implemented:

- **File Transfer**: DCC (Direct Client-to-Client) file sharing
- **Bot Integration**: Automated response and channel management bot
- **Server Statistics**: Connection and usage monitoring
- **Advanced Modes**: Additional channel and user modes
- **Logging System**: Comprehensive server activity logging

## ⚠️ Technical Notes

### C++98 Compliance
- All code must compile with `-std=c++98`
- No external libraries except standard C++ library
- No Boost or modern C++ features

### Network Requirements
- Only poll() (or select/kqueue/epoll) for I/O multiplexing
- All file descriptors must be non-blocking
- No read/recv or write/send without poll()
- Proper handling of EAGAIN/EWOULDBLOCK errors

### Error Handling
- Server must never crash or hang
- Graceful handling of low memory conditions
- Proper cleanup on client disconnection
- Comprehensive error logging

---

*This project is part of the 42 School Common Core curriculum, focusing on network programming and systems administration.*