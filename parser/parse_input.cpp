#include "parse_input.hpp"

std::vector<std::string> InputParser::XtractInput(std::string& buf)
{
    std::vector<std::string> FullCmd; //store all the full cmd
    size_t cmd_pos;
    while ((cmd_pos = buf.find("\r\n")) != std::string::npos) //loop to find the end of the next cmd
    {
        std::string cmd = buf.substr(0,cmd_pos); //this extract cms to the end of the next cmd
        FullCmd.push_back(cmd);
        buf.erase(0, cmd_pos + 2); //delete all processsed cmd  from the buffer
    }
    return FullCmd; //finally return full cmd extracted
}