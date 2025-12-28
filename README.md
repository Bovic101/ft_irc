# ft_irc — C++ IRC Server (42)

A lightweight **IRC-style server** written in **C++** for Linux.  
Supports **multiple concurrent clients** and **real-time messaging** using **TCP socket programming**, **non-blocking I/O**, and **`poll()`** for efficient I/O handling.

---

## Features

### Concurrency / I/O model
- TCP server lifecycle: `socket()` → `bind()` → `listen()` → `accept()`
- Multi-client support with **I/O multiplexing** using **`poll()`**
- **Non-blocking sockets** via `fcntl(fd, F_SETFL, O_NONBLOCK)`
- Reusable port on restart using `setsockopt(SO_REUSEADDR)`

### Implemented IRC-style commands
- Authentication / registration: `PASS`, `USER`, `NICK`
- Channels: `JOIN`, `PART`, `LIST`
- Messaging: `PRIVMSG` (user or channel)
- Channel management: `INVITE`, `KICK`, `TOPIC`, `MODE`
- Exit: `QUIT`

### Parsing & buffering
- Per-client buffering to handle partial reads (`Client::_buffer`)
- Extracts complete lines split by `\n` and strips optional `\r`
- Command parsing includes:
  - command normalization to uppercase
  - whitespace tokenization
  - support for `:` trailing-parameter style (IRC-like)

---

## Build

### Requirements
- Linux and C++ experience
- C++ compiler

### Compile
Build the program 
```bash
make

##Run
```bash
./ircserv <port> <password>
