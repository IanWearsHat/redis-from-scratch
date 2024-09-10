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

// Repeatedly reads from fd into buf until n bytes are reached
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t) rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Repeatedly writes from buf into fd until n bytes are written
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t) rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// const size_t k_max_msg = 4096;
int32_t process_client(int connfd) {
    // Read 4 byte header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cout << "read() error" << std::endl;
        }
        return err;
    }

    // Get length of message from header
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        std::cout << "too long" << std::endl;
        return -1;
    }

    // Read the number of bytes equal to length of message taken from header
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        std::cout << "read() error" << std::endl;
        return err;
    }

    // Add end of line to buffer so printf can print
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t) strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);

    return 0;
}

static void set_fd_to_nonblocking(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void) fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

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
    // while (true) {
    //     int connfd;

    //     struct sockaddr_in client_addr;
    //     int client_addr_len = sizeof(client_addr);

    //     std::cout << "Waiting for a client to connect...\n";

    //     connfd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    //     if (connfd < 0) {
    //         die("Accept client connection failed");
    //     }
    //     if (connfd >= 0) std::cout << "Client connected\n";

    //     while (true) {
    //         int32_t err = process_client(connfd);
    //         if (err) {
    //             break;
    //         }
    //     }

    //     close(connfd);
    // }

    close(server_fd);

    return 0;
}
