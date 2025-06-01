#include "parse_input.hpp"
#include <cctype>

// Default constructor
ParseCmd::ParseCmd() = default;

// Constructor with command and arguments
ParseCmd::ParseCmd(const std::string& cmd, const std::vector<std::string>& arguments)
    : command(cmd), args(arguments) {}

// Copy constructor
ParseCmd::ParseCmd(const ParseCmd& other)
    : command(other.command), args(other.args) {}

// Copy assignment operator
ParseCmd& ParseCmd::operator=(const ParseCmd& other) {
    if (this != &other) {
        command = other.command;
        args = other.args;
    }
    return *this;
}

// Move constructor
ParseCmd::ParseCmd(ParseCmd&& other) noexcept
    : command(std::move(other.command)), args(std::move(other.args)) {}

// Move assignment operator
ParseCmd& ParseCmd::operator=(ParseCmd&& other) noexcept {
    if (this != &other) {
        command = std::move(other.command);
        args = std::move(other.args);
    }
    return *this;
}

// Destructor
ParseCmd::~ParseCmd() = default;

// ✅ FIXED: Accept lines ending in \n or \r\n from the buffer
std::vector<std::string> InputParser::XtractInput(std::string& buf) {
    std::vector<std::string> FullCmd;
    size_t pos;

    while ((pos = buf.find('\n')) != std::string::npos) {
        std::string line = buf.substr(0, pos);

        // Remove optional trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        FullCmd.push_back(line);
        buf.erase(0, pos + 1);
    }

    return FullCmd;
}

// Splits a command line into tokens (command + args)
std::vector<std::string> InputParser::splitCmd(const std::string& line) {
    std::vector<std::string> tokens;
    size_t i = 0;
    const size_t len = line.length();

    while (i < len) {
        while (i < len && std::isspace(static_cast<unsigned char>(line[i]))) // Skip leading whitespace
            ++i;

        if (i >= len)
            break;

        if (line[i] == ':') {
            tokens.push_back(line.substr(i + 1));
            break;
        }

        size_t j = i;
        while (j < len && !std::isspace(static_cast<unsigned char>(line[j])))
            ++j;

        tokens.push_back(line.substr(i, j - i));
        i = j;
    }

    return tokens;
}

// Parses a full IRC line like "NICK john" into a ParseCmd object
ParseCmd InputParser::parseCommand(const std::string& line) {
    std::vector<std::string> tokens = splitCmd(line);

    if (!tokens.empty()) {
        std::string cmd = tokens[0];     // First token is the command
        tokens.erase(tokens.begin());   // Remaining are arguments
        return ParseCmd(cmd, tokens);
    }

    return ParseCmd();
}

