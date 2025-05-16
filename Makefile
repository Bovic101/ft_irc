# Compiler and flags
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -Iinclude

# Files and directories
NAME    = ircserv
SRCS    = main.cpp src/Server.cpp src/Client.cpp 
OBJS    = $(SRCS:.cpp=.o)

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
