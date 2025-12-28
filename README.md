# ft_irc project (IRC Server built in C++)

A lightweight IRC-style server project written in C++ for Linux, supporting multiple concurrent clients and real-time messaging using TCP sockets, non-blocking I/O, and poll() for efficient event handling.

**Requirements:** 
- Linux and C++ programming experience


## Run

Copy and paste these commands to build and run the server, then connect with `netcat`.

```bash
# 1) Build the project
Use the command "make"
This compile and produces the executable `ircserv` 

# 2) Start the server (replace port/password as needed)
`./ircserv <port> <password>`
Example
./ircserv 6667 pass

# 3) Open another terminal,connect using netcat to validate multi-client behaviour: 

nc localhost 6667
or 
nc 127.0.0.1 6667

Try these commands: 
PASS pass
USER victor 0 * :Victor Bobo
NICK Bovic
JOIN #general
PRIVMSG #general :hello guys
LIST
TOPIC #general :Music room
PART #general
QUIT :Thank you
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
