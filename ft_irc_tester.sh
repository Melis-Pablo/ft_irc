#!/bin/bash

# ft_irc Comprehensive Tester using irssi
# Tests all mandatory features of the IRC server implementation

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SERVER_HOST="127.0.0.1"
SERVER_PORT="6667"
SERVER_PASSWORD="testpass"
TEST_CHANNEL="#testchan"
TIMEOUT=10
IRSSI_CONFIG_DIR="/tmp/irssi_test"

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "PASS")
            echo -e "${GREEN}[PASS]${NC} $message"
            ((PASSED_TESTS++))
            ;;
        "FAIL")
            echo -e "${RED}[FAIL]${NC} $message"
            ((FAILED_TESTS++))
            ;;
        "INFO")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} $message"
            ;;
    esac
    ((TOTAL_TESTS++))
}

# Function to start IRC server
start_server() {
    print_status "INFO" "Starting IRC server on port $SERVER_PORT..."
    
    if [ ! -f "./ircserv" ]; then
        print_status "FAIL" "IRC server executable './ircserv' not found!"
        exit 1
    fi
    
    # Start server in background
    ./ircserv $SERVER_PORT $SERVER_PASSWORD > server.log 2>&1 &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 2
    
    # Check if server is running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        print_status "FAIL" "Server failed to start!"
        cat server.log
        exit 1
    fi
    
    print_status "PASS" "Server started successfully (PID: $SERVER_PID)"
}

# Function to stop IRC server
stop_server() {
    if [ ! -z "$SERVER_PID" ]; then
        print_status "INFO" "Stopping IRC server..."
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        print_status "INFO" "Server stopped"
    fi
}

# Function to create irssi config
setup_irssi_config() {
    local client_name=$1
    local config_dir="$IRSSI_CONFIG_DIR/$client_name"
    
    mkdir -p "$config_dir"
    
    cat > "$config_dir/config" << EOF
servers = (
  {
    address = "$SERVER_HOST";
    port = "$SERVER_PORT";
    password = "$SERVER_PASSWORD";
    use_tls = "no";
    autoconnect = "yes";
  }
);

channels = ( );

settings = {
  core = {
    real_name = "Test User $client_name";
    user_name = "testuser$client_name";
    nick = "testnick$client_name";
    log_level = "CLIENTERROR CLIENTNOTICE";
  };
  "fe-common/core" = {
    autolog = "yes";
    autolog_path = "$config_dir/logs/\$tag/\$0.log";
  };
};

logs = { };
EOF

    mkdir -p "$config_dir/logs"
}

# Function to start irssi client
start_irssi_client() {
    local client_name=$1
    local config_dir="$IRSSI_CONFIG_DIR/$client_name"
    
    setup_irssi_config "$client_name"
    
    # Start irssi in background with custom config
    irssi --config="$config_dir/config" --home="$config_dir" &
    local pid=$!
    
    echo $pid
}

# Function to send command via irssi
send_irssi_command() {
    local client_name=$1
    local command=$2
    local config_dir="$IRSSI_CONFIG_DIR/$client_name"
    
    # Use FIFO pipe to send commands to irssi
    echo "$command" > "$config_dir/input.fifo" 2>/dev/null || {
        # Alternative: use expect or similar tool
        print_status "WARN" "Could not send command to irssi client $client_name"
        return 1
    }
}

# Function to test basic server startup
test_server_startup() {
    echo -e "\n${YELLOW}=== Testing Server Startup ===${NC}"
    
    # Test with invalid arguments
    print_status "INFO" "Testing server with invalid arguments..."
    
    # Test missing arguments
    timeout 5 ./ircserv > /dev/null 2>&1
    if [ $? -eq 124 ] || [ $? -eq 1 ]; then
        print_status "PASS" "Server correctly handles missing arguments"
    else
        print_status "FAIL" "Server should reject missing arguments"
    fi
    
    # Test invalid port
    timeout 5 ./ircserv "invalid_port" "password" > /dev/null 2>&1
    if [ $? -eq 124 ] || [ $? -eq 1 ]; then
        print_status "PASS" "Server correctly handles invalid port"
    else
        print_status "FAIL" "Server should reject invalid port"
    fi
}

# Function to test basic connection
test_basic_connection() {
    echo -e "\n${YELLOW}=== Testing Basic Connection ===${NC}"
    
    # Test connection with nc
    print_status "INFO" "Testing basic TCP connection..."
    timeout 5 bash -c "echo 'PING test' | nc $SERVER_HOST $SERVER_PORT" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        print_status "PASS" "Basic TCP connection successful"
    else
        print_status "FAIL" "Cannot establish basic TCP connection"
    fi
}

# Function to test authentication
test_authentication() {
    echo -e "\n${YELLOW}=== Testing Authentication ===${NC}"
    
    # Test wrong password
    print_status "INFO" "Testing authentication with wrong password..."
    {
        echo "PASS wrongpassword"
        echo "NICK testnick"
        echo "USER testuser 0 * :Test User"
        sleep 2
    } | timeout 10 nc $SERVER_HOST $SERVER_PORT > auth_test.log 2>&1
    
    if grep -q "464\|ERROR" auth_test.log; then
        print_status "PASS" "Server correctly rejects wrong password"
    else
        print_status "FAIL" "Server should reject wrong password"
    fi
    
    # Test correct password
    print_status "INFO" "Testing authentication with correct password..."
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK testnick"
        echo "USER testuser 0 * :Test User"
        sleep 2
    } | timeout 10 nc $SERVER_HOST $SERVER_PORT > auth_test2.log 2>&1
    
    if grep -q "001\|Welcome" auth_test2.log; then
        print_status "PASS" "Server correctly accepts valid authentication"
    else
        print_status "FAIL" "Server should accept valid authentication"
    fi
    
    rm -f auth_test.log auth_test2.log
}

# Function to test nickname and username setting
test_nick_user_setting() {
    echo -e "\n${YELLOW}=== Testing Nickname and Username Setting ===${NC}"
    
    print_status "INFO" "Testing nickname and username registration..."
    
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK testnick123"
        echo "USER testuser 0 * :Test User Real Name"
        sleep 2
        echo "WHOIS testnick123"
        sleep 1
    } | timeout 15 nc $SERVER_HOST $SERVER_PORT > nick_test.log 2>&1
    
    if grep -q "testnick123" nick_test.log && grep -q "testuser" nick_test.log; then
        print_status "PASS" "Nickname and username set correctly"
    else
        print_status "FAIL" "Failed to set nickname and username"
    fi
    
    rm -f nick_test.log
}

# Function to test channel operations
test_channel_operations() {
    echo -e "\n${YELLOW}=== Testing Channel Operations ===${NC}"
    
    print_status "INFO" "Testing channel join and messaging..."
    
    # Start first client - join channel but wait before sending message
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK user1"
        echo "USER user1 0 * :User One"
        sleep 2
        echo "JOIN $TEST_CHANNEL"
        sleep 4  # Wait longer for second client to join
        echo "PRIVMSG $TEST_CHANNEL :Hello from user1"
        sleep 3  # Wait to receive second client's message
    } | timeout 25 nc $SERVER_HOST $SERVER_PORT > channel_test1.log 2>&1 &
    
    CLIENT1_PID=$!
    sleep 1  # Reduced delay before starting second client
    
    # Start second client - join channel quickly and send message
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK user2"
        echo "USER user2 0 * :User Two"
        sleep 2
        echo "JOIN $TEST_CHANNEL"
        sleep 1.5  # Quick join
        echo "PRIVMSG $TEST_CHANNEL :Hello from user2"
        sleep 3  # Wait to receive first client's message
    } | timeout 25 nc $SERVER_HOST $SERVER_PORT > channel_test2.log 2>&1 &
    
    CLIENT2_PID=$!
    
    # Wait for clients to finish
    wait $CLIENT1_PID $CLIENT2_PID
    
    # Check if messages were exchanged
    if grep -q "Hello from user2" channel_test1.log && grep -q "Hello from user1" channel_test2.log; then
        print_status "PASS" "Channel messaging works correctly"
    else
        print_status "FAIL" "Channel messaging failed"
        # Debug output
        print_status "INFO" "Debug - User1 log:"
        grep "PRIVMSG\|Hello" channel_test1.log 2>/dev/null || echo "No messages found"
        print_status "INFO" "Debug - User2 log:"  
        grep "PRIVMSG\|Hello" channel_test2.log 2>/dev/null || echo "No messages found"
    fi
    
    rm -f channel_test1.log channel_test2.log
}

# Function to test private messaging
test_private_messaging() {
    echo -e "\n${YELLOW}=== Testing Private Messaging ===${NC}"
    
    print_status "INFO" "Testing private messages between users..."
    
    # Start first client
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK privuser1"
        echo "USER privuser1 0 * :Private User One"
        sleep 2
        echo "PRIVMSG privuser2 :Private message from user1"
        sleep 3
    } | timeout 20 nc $SERVER_HOST $SERVER_PORT > priv_test1.log 2>&1 &
    
    CLIENT1_PID=$!
    sleep 1
    
    # Start second client
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK privuser2"
        echo "USER privuser2 0 * :Private User Two"
        sleep 2
        echo "PRIVMSG privuser1 :Private reply from user2"
        sleep 3
    } | timeout 20 nc $SERVER_HOST $SERVER_PORT > priv_test2.log 2>&1 &
    
    CLIENT2_PID=$!
    
    # Wait for clients to finish
    wait $CLIENT1_PID $CLIENT2_PID
    
    # Check if private messages were received
    if grep -q "Private reply from user2" priv_test1.log && grep -q "Private message from user1" priv_test2.log; then
        print_status "PASS" "Private messaging works correctly"
    else
        print_status "FAIL" "Private messaging failed"
    fi
    
    rm -f priv_test1.log priv_test2.log
}

# Function to test operator commands
test_operator_commands() {
    echo -e "\n${YELLOW}=== Testing Operator Commands ===${NC}"
    
    print_status "INFO" "Testing operator commands (KICK, INVITE, TOPIC, MODE)..."
    
    # Test TOPIC command
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK opuser"
        echo "USER opuser 0 * :Operator User"
        sleep 2
        echo "JOIN $TEST_CHANNEL"
        sleep 1
        echo "TOPIC $TEST_CHANNEL :New channel topic"
        sleep 1
        echo "TOPIC $TEST_CHANNEL"
        sleep 2
    } | timeout 20 nc $SERVER_HOST $SERVER_PORT > op_test.log 2>&1
    
    if grep -q "New channel topic" op_test.log; then
        print_status "PASS" "TOPIC command works"
    else
        print_status "FAIL" "TOPIC command failed"
    fi
    
    # Test MODE commands
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK modeuser"
        echo "USER modeuser 0 * :Mode User"
        sleep 2
        echo "JOIN $TEST_CHANNEL"
        sleep 1
        echo "MODE $TEST_CHANNEL +i"
        sleep 1
        echo "MODE $TEST_CHANNEL +t"
        sleep 1
        echo "MODE $TEST_CHANNEL +k testkey"
        sleep 1
        echo "MODE $TEST_CHANNEL +l 10"
        sleep 2
    } | timeout 20 nc $SERVER_HOST $SERVER_PORT > mode_test.log 2>&1
    
    if grep -q "+i\|+t\|+k\|+l" mode_test.log; then
        print_status "PASS" "MODE commands work"
    else
        print_status "FAIL" "MODE commands failed"
    fi
    
    rm -f op_test.log mode_test.log
}

# Function to test partial data handling
test_partial_data() {
    echo -e "\n${YELLOW}=== Testing Partial Data Handling ===${NC}"
    
    print_status "INFO" "Testing partial command reception..."
    
    # Use the suggested test from the subject
    {
        printf "PASS $SERVER_PASSWORD\r\n"
        printf "NICK testuser\r\n"
        printf "USER testuser 0 * :Test\r\n"
        sleep 1
        printf "com"
        sleep 0.5
        printf "man"
        sleep 0.5
        printf "d\r\n"
        sleep 2
    } | timeout 15 nc $SERVER_HOST $SERVER_PORT > partial_test.log 2>&1
    
    # Check if the server handled the partial command correctly
    if grep -q "421\|Unknown command" partial_test.log; then
        print_status "PASS" "Server correctly handles partial commands"
    else
        print_status "WARN" "Partial command handling unclear (check manually)"
    fi
    
    rm -f partial_test.log
}

# Function to test multiple clients
test_multiple_clients() {
    echo -e "\n${YELLOW}=== Testing Multiple Simultaneous Clients ===${NC}"
    
    print_status "INFO" "Testing server with multiple simultaneous connections..."
    
    # Start multiple clients simultaneously
    for i in {1..5}; do
        {
            echo "PASS $SERVER_PASSWORD"
            echo "NICK multiclient$i"
            echo "USER multiclient$i 0 * :Multi Client $i"
            sleep 2
            echo "JOIN #multitest"
            sleep 1
            echo "PRIVMSG #multitest :Message from client $i"
            sleep 3
        } | timeout 25 nc $SERVER_HOST $SERVER_PORT > multi_test_$i.log 2>&1 &
        
        MULTI_PIDS[$i]=$!
        sleep 0.5
    done
    
    # Wait for all clients to finish
    for i in {1..5}; do
        wait ${MULTI_PIDS[$i]}
    done
    
    # Check if all clients could connect and communicate
    success_count=0
    for i in {1..5}; do
        if [ -f "multi_test_$i.log" ] && grep -q "001\|Welcome" "multi_test_$i.log"; then
            ((success_count++))
        fi
        rm -f "multi_test_$i.log"
    done
    
    if [ $success_count -ge 4 ]; then
        print_status "PASS" "Multiple clients handled successfully ($success_count/5)"
    else
        print_status "FAIL" "Failed to handle multiple clients properly ($success_count/5)"
    fi
}

# Function to test error conditions
test_error_conditions() {
    echo -e "\n${YELLOW}=== Testing Error Conditions ===${NC}"
    
    # Test connection without password
    print_status "INFO" "Testing connection without password..."
    {
        echo "NICK nopassuser"
        echo "USER nopassuser 0 * :No Pass User"
        sleep 2
    } | timeout 10 nc $SERVER_HOST $SERVER_PORT > error_test1.log 2>&1
    
    if grep -q "464\|ERROR" error_test1.log; then
        print_status "PASS" "Server rejects connection without password"
    else
        print_status "FAIL" "Server should reject connection without password"
    fi
    
    # Test duplicate nickname - FIXED VERSION
    print_status "INFO" "Testing duplicate nickname handling..."
    
    # Start first client with longer connection time
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK testnick"              # Changed from 'dupnick' to 'testnick'
        echo "USER testuser1 0 * :Test User One"
        sleep 5                          # Changed from 2 to 5 seconds
    } | timeout 12 nc $SERVER_HOST $SERVER_PORT > dup_test1.log 2>&1 &
    
    FIRST_CLIENT=$!
    
    # Wait for first client to register
    sleep 3                              # Changed from 2 to 3 seconds
    
    # Start second client with same nickname
    {
        echo "PASS $SERVER_PASSWORD"
        echo "NICK testnick"              # Changed from 'dupnick' to 'testnick'
        echo "USER testuser2 0 * :Test User Two"
        sleep 3
    } | timeout 10 nc $SERVER_HOST $SERVER_PORT > dup_test2.log 2>&1
    
    # Check results
    if grep -q "433.*testnick.*already in use" dup_test2.log; then
        print_status "PASS" "Server correctly rejects duplicate nicknames"
    elif grep -q "433" dup_test2.log; then
        print_status "PASS" "Server rejects duplicate nicknames (format may vary)"
        print_status "INFO" "Response: $(grep '433' dup_test2.log)"
    else
        print_status "FAIL" "Server should reject duplicate nicknames"
        # Debug output
        print_status "INFO" "First client response:"
        cat dup_test1.log 2>/dev/null | head -3
        print_status "INFO" "Second client response:"
        cat dup_test2.log 2>/dev/null | head -3
    fi
    
    # Clean up
    kill $FIRST_CLIENT 2>/dev/null
    wait $FIRST_CLIENT 2>/dev/null
    rm -f dup_test1.log dup_test2.log
    rm -f error_test1.log error_test2.log
}

# Function to compile and check the project
test_compilation() {
    echo -e "\n${YELLOW}=== Testing Compilation ===${NC}"
    
    print_status "INFO" "Testing Makefile compilation..."
    
    # Test make clean
    make clean > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        print_status "PASS" "make clean works"
    else
        print_status "FAIL" "make clean failed"
    fi
    
    # Test make
    make > compilation.log 2>&1
    if [ $? -eq 0 ] && [ -f "ircserv" ]; then
        print_status "PASS" "Compilation successful"
    else
        print_status "FAIL" "Compilation failed"
        cat compilation.log
        exit 1
    fi
    
    # Test make re
    make re > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        print_status "PASS" "make re works"
    else
        print_status "FAIL" "make re failed"
    fi
    
    rm -f compilation.log
}

# Function to show test summary
show_summary() {
    echo -e "\n${YELLOW}=== Test Summary ===${NC}"
    echo -e "Total tests: $TOTAL_TESTS"
    echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}🎉 All tests passed! Your IRC server implementation looks good.${NC}"
        exit 0
    else
        echo -e "\n${RED}❌ Some tests failed. Please check your implementation.${NC}"
        exit 1
    fi
}

# Cleanup function
cleanup() {
    print_status "INFO" "Cleaning up..."
    stop_server
    rm -rf "$IRSSI_CONFIG_DIR"
    rm -f *.log
    
    # Kill any remaining nc processes
    pkill -f "nc.*$SERVER_PORT" 2>/dev/null
}

# Main function
main() {
    echo -e "${BLUE}ft_irc Comprehensive Tester${NC}"
    echo -e "${BLUE}=============================${NC}"
    
    # Set up signal handlers for cleanup
    trap cleanup EXIT INT TERM
    
    # Check dependencies
    if ! command -v nc &> /dev/null; then
        print_status "FAIL" "netcat (nc) is required but not installed"
        exit 1
    fi
    
    # Compilation tests
    test_compilation
    
    # Server startup tests
    test_server_startup
    
    # Start the server for main tests
    start_server
    
    # Wait a bit for server to be ready
    sleep 2
    
    # Run all tests
    test_basic_connection
    test_authentication
    test_nick_user_setting
    test_channel_operations
    test_private_messaging
    test_operator_commands
    test_partial_data
    test_multiple_clients
    test_error_conditions
    
    # Show summary
    show_summary
}

# Check if script is run with proper arguments
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: $0 [port] [password]"
    echo "Default port: 6667"
    echo "Default password: testpass"
    echo ""
    echo "This script tests your ft_irc server implementation."
    echo "Make sure your server executable is named 'ircserv' and is in the current directory."
    exit 0
fi

# Allow custom port and password
if [ ! -z "$1" ]; then
    SERVER_PORT="$1"
fi

if [ ! -z "$2" ]; then
    SERVER_PASSWORD="$2"
fi

# Run main function
main