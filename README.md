# ft_irc — C++ IRC Server

A lightweight IRC-style server implemented in C++ for learning network and systems programming. Supports multiple concurrent clients, channels, and a subset of IRC commands.

**Build**

**Requirements:** 
- Linux and C++ experience

### Compile
If you have a Makefile:
```bash
make


Run:

```bash
make
```

This produces the executable `ircserv` (see [Makefile](Makefile)).

## Quick Start — Copy & Paste

Copy and paste these commands to build and run the server, then connect with `netcat`.

```bash
# 1) Build the project
make

# 2) Start the server (replace port/password as needed)
./ircserv 6667 secretpass

# 3) In another terminal, connect using netcat (or use an IRC client)
nc localhost 6667

# Example: send raw commands (replace with proper IRC flow)
# PASS pass
# NICK Bovic
# USER newuser 0 * :Bobo Victor
```

## Usage

`./ircserv <port> <password>`

Example:

```bash
./ircserv 6667 pass
```

## Project structure

- `main.cpp` — program entry, argument parsing, starts `Server`.
- `include/Server.hpp`, `src/Server.cpp` — core networking and event loop.
- `include/Client.hpp`, `src/Client.cpp` — per-client logic and buffering.
- `include/channel.hpp`, `src/channel.cpp` — channel management.
- `include/parse_input.hpp`, `src/parse_input.cpp` — command parsing.
- `Makefile` — build targets: `make`, `make clean`, `make fclean`, `make re`.
- `test_irc.sh` — helper script for manual testing.


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
