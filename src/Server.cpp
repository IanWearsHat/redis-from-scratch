#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>

#include "connections.h"
#include <vector>

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;


    // Create socket - AF_INET for IPV4, SOCK_STREAM for TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        die("Failed to create server socket\n");
    }

    // Configure socket
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        die("setsockopt failed\n");
    }

    // Bind to address 6379
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(6379);

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        die("Failed to bind to port 6379\n");
    }

    // Enable listening for connections
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        die("listen failed\n");
    }

    set_fd_to_nonblocking(server_fd);

    run_event_loop(server_fd);

    close(server_fd);

    return 0;
}
