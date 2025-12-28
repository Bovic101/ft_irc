ft_irc — Simple IRC-like server (C++)

**Project**
- **Description:** Minimal IRC-like server implemented in C++ as an educational project. It accepts client connections, manages channels, and parses basic IRC commands.

**Build**
- **Requirements:** `c++` compiler (C++17), `make`.
- **Build:** run `make` which produces the executable `ircserv` (see [Makefile](Makefile)).
- **Clean:** `make clean`, `make fclean`, `make re`.

**Run**
- **Usage:** `./ircserv <port> <password>` — starts the server on the given TCP port with the provided server password.
- **Example:** `./ircserv 6667 secretpass`

**Usage**
- Connect an IRC-capable client (or `telnet`/`netcat`) to the server port and authenticate using the server password when required by the command flow implemented by the server.

**Code Structure**
- **Entry point:** [main.cpp](main.cpp) — parses arguments and starts the `Server`.
- **Core server:** [src/Server.cpp](src/Server.cpp) and [include/Server.hpp](include/Server.hpp) — main networking and event loop.
- **Client handling:** [src/Client.cpp](src/Client.cpp) and [include/Client.hpp](include/Client.hpp).
- **Channels:** [src/channel.cpp](src/channel.cpp) and [include/channel.hpp](include/channel.hpp).
- **Input parsing:** [src/parse_input.cpp](src/parse_input.cpp) and [include/parse_input.hpp](include/parse_input.hpp).

**Protocol Notes**
- This project implements a subset of IRC functionality (channels, basic commands and message routing). It is intended for learning systems/network programming rather than production use.

**Testing**
- A helper script `test_irc.sh` is included for quick manual tests; run `./test_irc.sh` to exercise common flows (ensure `ircserv` is built).

**Contributing**
- Fork, make changes, and open a pull request. Keep changes focused and provide short descriptions of behavior changes.

**License**
- No license specified in this repository. Contact the author or add a license file if you want to distribute the code.

