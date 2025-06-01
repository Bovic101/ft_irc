#ifndef PARSE_INPUT_HPP
#define PARSE_INPUT_HPP

#include <string>
#include <vector>
#include <cctype>
#include <iostream>

// This Stores parsed command and its arguments
class ParseCmd {
public:
    ParseCmd();
    ParseCmd(const std::string& cmd, const std::vector<std::string>& arguments); // Constructor with values
    ParseCmd(const ParseCmd& other);
    ParseCmd& operator=(const ParseCmd& other);
    ParseCmd(ParseCmd&& other) noexcept;
    ParseCmd& operator=(ParseCmd&& other) noexcept; // Move assignment
    ~ParseCmd();

    std::string command; // This take in cmd like NICK, JOIN
    std::vector<std::string> args; // cmd arguments
};

class InputParser {
public:
    static std::vector<std::string> XtractInput(std::string& buf); // Extracts complete lines ending in \r\n
    static std::vector<std::string> splitCmd(const std::string& line); // Splits a line into command and args
    static ParseCmd parseCommand(const std::string& line); // Parses a full IRC line into a structured command
};

#endif
