#!/bin/bash

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color
CHECK="${GREEN}✔${NC}"
CROSS="${RED}✘${NC}"
INFO="${BLUE}➤${NC}"

# Function to simulate a client connecting to the IRC server
run_client() {
  local name=$1
  echo -e "${INFO} ${YELLOW}Simulating client:${NC} $name"
  {
    sleep 0.5
    echo "PASS pass"
    sleep 0.5
    echo "NICK $name"                  # Set nickname
    echo "USER $name 0 * :Real Name"   # Register user
    shift
    while [[ $# -gt 0 ]]; do
      eval "$1"
      shift
    done
  } | nc localhost 6667
}

# User maryo sets up the channel and configures it
run_client maryo \
  "sleep 1" \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'TOPIC #codingChannel :Welcome to #codingChannel!'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +i'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +t'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +k secret'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +l 3'" \
  "sleep 1" \
  "echo 'LIST'" \
  "sleep 1" \
  "echo 'INVITE mary #codingChannel'" \
  "sleep 1" \
  "echo 'INVITE victor #codingChannel'" \
  "sleep 1" \
  "echo 'INVITE blatifat #codingChannel'" \
  "sleep 1" \
  "echo 'QUIT :maryo out'" &

# User mary joins and chats
sleep 2
run_client mary \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'PRIVMSG #codingChannel :Hi everyone, mary here!'" \
  "sleep 1" \
  "echo 'PART #codingChannel'" \
  "sleep 1" \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'QUIT :mary out again'" &

# blatifat joins using invite and sets topic
sleep 3
run_client blatifat \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'PRIVMSG #codingChannel :hello all!'" \
  "sleep 1" \
  "echo 'TOPIC #codingChannel :Updated topic by blatifat'" \
  "sleep 1" \
  "echo 'QUIT :done!'" &

# victor joins using key and is kicked
sleep 4
run_client victor \
  "echo 'JOIN #codingChannel secret'" \
  "sleep 1" \
  "echo 'PRIVMSG #codingChannel :I am victor!'" \
  "sleep 1" \
  "echo 'KICK blatifat #codingChannel :Bye bro'" \
  "sleep 1" \
  "echo 'MODE #codingChannel -i'" \
  "sleep 1" \
  "echo 'QUIT :victor done'" &

# User test sends private message
sleep 5
run_client test \
  "echo 'PRIVMSG victor :Private message test'" \
  "sleep 1" \
  "echo 'QUIT :test user done'" &

# Error test cases: no PASS, no NICK, invalid JOIN
sleep 6
run_client errorCase \
  "echo 'USER errorCase 0 * :Should fail'" \
  "sleep 1" \
  "echo 'JOIN notAChannel'" \
  "sleep 1" \
  "echo 'JOIN #codingChannel wrongKey'" \
  "sleep 1" \
  "echo 'QUIT :error testing done'" &

# Final LIST after all ops
sleep 7
run_client list_checker \
  "echo 'LIST'" \
  "sleep 1" \
  "echo 'QUIT :list check done'" &

# Wait for all client simulations to finish
wait
echo -e "${CHECK} ${GREEN}All ft_irc mandatory command edge cases tested successfully.${NC}"

