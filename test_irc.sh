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
    sleep 1
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
  "sleep 1.2" \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'TOPIC #codingChannel :Welcome to #codingChannel!'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +i'" \
  "sleep 0.9" \
  "echo 'MODE #codingChannel +t'" \
  "sleep 0.9" \
  "echo 'MODE #codingChannel +k secret'" \
  "sleep 1" \
  "echo 'MODE #codingChannel +l 5'" \
  "sleep 0.8" \
  "echo 'LIST'" \
  "sleep 0.8" \
  "echo 'INVITE mary #codingChannel'" \
  "sleep 1" \
  "echo 'QUIT :maryo out'" &

# User mary joins after invite and sends a message
sleep 2
run_client mary \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'PRIVMSG #codingChannel :Hi everyone, mary here!'" \
  "sleep 1" \
  "echo 'QUIT :mary says bye'" &

# blatifat tries to join without invite (should fail)
sleep 3
run_client blatifat \
  "echo 'JOIN #codingChannel'" \
  "sleep 1" \
  "echo 'QUIT :blatifat failed to join'" &

# victor joins using the key and is kicked
sleep 4
run_client victor \
  "echo 'JOIN #codingChannel secret'" \
  "sleep 1" \
  "echo 'PRIVMSG #codingChannel :victor joined'" \
  "sleep 1" \
  "echo 'KICK victor #codingChannel :You'\''re out'" \
  "sleep 1" \
  "echo 'QUIT :victor kicked out'" &

# List channels again
sleep 5
run_client list_checker \
  "echo 'LIST'" \
  "sleep 1" \
  "echo 'QUIT :list check done'" &

# Wait for all client simulations to finish
wait
echo -e "${CHECK} ${GREEN}All IRC command tests are fully completed. Congrats!${NC}"
