#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <poll.h>

#include "connections.h"

void die(const char* msg) {
    std::cerr << msg << std::endl;
    exit(1);
}

int accept_new_connection(int server_fd, std::vector<Conn *> &fd2conn) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &socklen);
    if (client_fd < 0) {
        die("accept error");
    }

    return client_fd;
}

void resize_connections(int client_fd, std::vector<Conn *> &fd2conn) {
    if (client_fd > fd2conn.size()) {
        fd2conn.resize(client_fd + 1);
    }
}

void map_new_connection(int client_fd, std::vector<Conn *> &fd2conn) {
    Conn* new_connection = new Conn();
    new_connection->fd = client_fd;
    fd2conn[client_fd] = new_connection;
}

void run_event_loop(int server_fd) {
    std::vector<Conn *> fd2conn;

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true) {
        // prepare the arguments of the poll()
        poll_args.clear();

        // for convenience, the listening fd is put in the first position
        // For this fd, only wait for a POLLIN (flag if there is data to read). Set revents to 0 as default.
        struct pollfd pfd = {server_fd, POLLIN, 0};
        poll_args.push_back(pfd);

        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), 1000);

        if (poll_args[0].revents) {
            int client_fd = accept_new_connection(server_fd, fd2conn);
            resize_connections(client_fd, fd2conn);
            map_new_connection(client_fd, fd2conn);

            for (Conn* conn: fd2conn) {
                if (conn) {
                    std::cout << conn->fd << std::endl;
                }
            }
            exit(0);
        }
    }
}