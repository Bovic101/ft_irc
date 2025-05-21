#ifndef PARSE_INPUT_HPP
#define PARSE_INPUT_HPP

#include <iostream>
#include <string>
#include <vector>

class InputParser
{
    public:
        static std::vector<std::string>XtractInput(std::string& buf);

};

#endif