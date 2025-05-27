# ft_irc Project Complete Checklist

## 📋 General Rules Checklist

### Program Stability & Error Handling
- [ ] Program never crashes in any circumstances (even when out of memory)
- [ ] Program never quits unexpectedly
- [ ] Handle all possible errors and edge cases
- [ ] Verify handling of partial data reception
- [ ] Verify handling of low bandwidth scenarios
- [ ] Test with various error conditions

### Compilation & Build System
- [ ] Create Makefile that compiles all source files
- [ ] Makefile does not perform unnecessary relinking
- [ ] Makefile contains required rules:
  - [ ] `$(NAME)` rule
  - [ ] `all` rule  
  - [ ] `clean` rule
  - [ ] `fclean` rule
  - [ ] `re` rule
- [ ] Compile with `c++` compiler
- [ ] Use compilation flags: `-Wall -Wextra -Werror`
- [ ] Code compiles with `-std=c++98` flag
- [ ] All code complies with C++ 98 standard

### Code Standards & Libraries
- [ ] Always use C++ features when available (e.g., `<cstring>` over `<string.h>`)
- [ ] Prefer C++ versions of functions over C versions when possible
- [ ] No external libraries used
- [ ] No Boost libraries used
- [ ] Write clean, readable code

### File Submission Requirements
- [ ] Submit Makefile
- [ ] Submit all `.h` and `.hpp` header files
- [ ] Submit all `.cpp` source files
- [ ] Submit any `.tpp` template files (if used)
- [ ] Submit any `.ipp` implementation files (if used)
- [ ] Optional: Submit configuration file
- [ ] Ensure all file names are correct

---

## 🎯 Technical Requirements Checklist

### Core Program Structure
- [ ] Program name is `ircserv`
- [ ] Executable accepts exactly 2 arguments: `./ircserv <port> <password>`
  - [ ] `port`: The listening port number
  - [ ] `password`: Connection password for clients
- [ ] Develop IRC server (NOT client)
- [ ] Do NOT implement server-to-server communication

### Allowed Functions & System Calls
- [ ] Use only C++ 98 standard library functions
- [ ] Allowed system functions:
  - [ ] `socket, close, setsockopt, getsockname`
  - [ ] `getprotobyname, gethostbyname`
  - [ ] `getaddrinfo, freeaddrinfo`
  - [ ] `bind, connect, listen, accept`
  - [ ] `htons, htonl, ntohs, ntohl`
  - [ ] `inet_addr, inet_ntoa`
  - [ ] `send, recv`
  - [ ] `signal, sigaction`
  - [ ] `lseek, fstat`
  - [ ] `fcntl, poll` (or equivalent)

### Network & I/O Architecture
- [ ] Handle multiple clients simultaneously without hanging
- [ ] **CRITICAL**: No forking allowed
- [ ] **CRITICAL**: All I/O operations must be non-blocking
- [ ] **CRITICAL**: Use only ONE `poll()` (or equivalent) for ALL operations
  - [ ] Handle read operations with poll
  - [ ] Handle write operations with poll  
  - [ ] Handle listen operations with poll
  - [ ] Handle all other I/O with same poll instance
- [ ] **CRITICAL**: Never use `read/recv` or `write/send` without `poll()` (automatic grade 0)
- [ ] Use TCP/IP (v4 or v6) for client-server communication

### Client Connection & Authentication
- [ ] Choose and document reference IRC client for testing
- [ ] Reference client must connect without errors
- [ ] Implement user authentication system
- [ ] Allow clients to set nickname
- [ ] Allow clients to set username
- [ ] Handle connection password verification

### Channel Management
- [ ] Allow clients to join channels
- [ ] Forward all channel messages to every client in that channel
- [ ] Implement user roles: operators and regular users
- [ ] Support private messages between clients

### Required IRC Commands - Operator Functions
- [ ] **KICK** command - Eject a client from the channel
- [ ] **INVITE** command - Invite a client to a channel  
- [ ] **TOPIC** command - Change or view the channel topic
- [ ] **MODE** command with following modes:
  - [ ] `i`: Set/remove Invite-only channel
  - [ ] `t`: Set/remove TOPIC command restrictions to operators only
  - [ ] `k`: Set/remove channel key (password)
  - [ ] `o`: Give/take channel operator privilege
  - [ ] `l`: Set/remove user limit to channel

### Data Processing & Protocol Compliance
- [ ] Aggregate received packets to rebuild complete commands
- [ ] Handle partial command reception (test with ctrl+D method)
- [ ] Process commands only when complete
- [ ] Follow IRC protocol standards for message formatting
- [ ] Handle command parsing correctly

### MacOS Specific Requirements (if applicable)
- [ ] Use `fcntl()` for non-blocking file descriptors on MacOS
- [ ] Only allowed `fcntl()` usage: `fcntl(fd, F_SETFL, O_NONBLOCK)`
- [ ] No other `fcntl()` flags permitted

### Testing & Validation
- [ ] Test with reference IRC client
- [ ] Perform partial data test using `nc`:
  ```bash
  nc -C 127.0.0.1 6667
  com^Dman^Dd
  ```
- [ ] Test multiple simultaneous client connections
- [ ] Test all operator commands
- [ ] Test channel message forwarding
- [ ] Test private messaging
- [ ] Test authentication system
- [ ] Test error handling and edge cases
- [ ] Verify non-blocking behavior under load

### Code Quality & Documentation
- [ ] Code is clean and well-organized
- [ ] Proper error handling throughout
- [ ] Memory management (no leaks)
- [ ] Consistent coding style
- [ ] Appropriate comments where needed

---

## 🚨 Critical Failure Points (Automatic Grade 0)
- [ ] **Avoided**: Program crashes under any circumstances
- [ ] **Avoided**: Program quits unexpectedly  
- [ ] **Avoided**: Using forking
- [ ] **Avoided**: Using blocking I/O operations
- [ ] **Avoided**: Using multiple poll() instances
- [ ] **Avoided**: Using read/recv or write/send without poll()
- [ ] **Avoided**: Using external libraries or Boost

---

## 📝 Bonus Features (Optional - Only if Mandatory is Perfect)
- [ ] Handle file transfer functionality
- [ ] Implement a bot

**Note**: Bonus features are only evaluated if ALL mandatory requirements are perfectly implemented and working without any issues.