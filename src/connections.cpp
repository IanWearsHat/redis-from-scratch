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
#include <assert.h>

#include "connections.h"

// Repeatedly reads from fd into buf until n bytes are reached
static int32_t read_full(int fd, uint8_t *buf, size_t n) {
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
static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
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

void try_process_one_request(Conn *client) {
    // Get length of message from header
    uint32_t len;
    memcpy(&len, client->last_request_end_pointer, 4);  // assume little endian
    if (len > k_max_msg) {
        std::cout << "too long" << std::endl;
        exit(1);
    }

    // either the entire message hasn't been read into the buffer or we have nothing to read
    // either way, we exit this loop
    if (4 + len > client->rbuf_size) {
        std::cout << "end maybe probably" << std::endl;
        exit(1);
    }

    // Read the number of bytes equal to length of message taken from header
    uint8_t *msg = (uint8_t *) malloc(len);
    if (msg != NULL) {
        memcpy(msg, &client->last_request_end_pointer[4], len);

        // do sth with payload
        printf("client says: %s\n", msg);

        memcpy(client->wbuf, &len, 4);
        memcpy(&client->wbuf[4], msg, len);

        client->wbuf_size += 4 + len;

        handle_response_state(client);

        free(msg);
    }

    // move pointer to end of request
    client->last_request_end_pointer = &client->last_request_end_pointer[4 + len];
    client->rbuf_size -= 4 + len;
}

void fill_read_buffer(Conn *client) {
    assert(client->rbuf_size < sizeof(client->rbuf));
    ssize_t rv = 0;

    memmove(client->rbuf, client->last_request_end_pointer, client->rbuf_size);
    client->last_request_end_pointer = client->rbuf;

    do {
        size_t cap = sizeof(client->rbuf) - client->rbuf_size;
        rv = read(client->fd, &client->rbuf[client->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN) {
        // mark connection for end
        // break out of while loop in handle_request_state
    }

    if (rv == 0) {
        // reached EOF
        // mark connection for end
        // break out of while loop in handle_request_state
    }

    client->rbuf_size += (size_t) rv;
    assert(client->rbuf_size <= sizeof(client->rbuf));
}

void handle_request_state(Conn *client) {
    fill_read_buffer(client);
    // try_process_one_request(client);
    while (true) {
        try_process_one_request(client);
    }
}

bool try_flush_buffer(Conn *client) {
    ssize_t rv = 0;
    do {
        size_t remain = client->wbuf_size - client->wbuf_sent;
        rv = write(client->fd, &client->wbuf[client->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        std::cout << "write() error" << std::endl;
        client->state = STATE_END;
        return false;
    }
    client->wbuf_sent += (size_t)rv;
    assert(client->wbuf_sent <= client->wbuf_size);
    if (client->wbuf_sent == client->wbuf_size) {
        // response was fully sent, change state back
        client->state = STATE_REQ;
        client->wbuf_sent = 0;
        client->wbuf_size = 0;
        return false;
    }
    // still got some data in wbuf, could try to write again
    return true;
}

void handle_response_state(Conn *client) {
    while (true) {
        try_flush_buffer(client);
    }
}

void perform_action_on_client(int client_fd, std::vector<Conn *> &fd2conn) {
    Conn* client = fd2conn[client_fd];
    if (client->state == STATE_REQ) {
        handle_request_state(client);
    } else if (client->state == STATE_RES) {
        handle_response_state(client);
    } else {
        // shouldn't be here
    }
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

        // add all connections from fd2conn to the poll set
        for (Conn* conn: fd2conn) {
            if (conn) {
                if (conn->fd == -1) {
                    std::cout << "File descriptor is -1" << std::endl;
                    continue;
                }

                if (conn->state == STATE_END) {
                    // end connection
                    // free resources
                    // update fd2conn

                    // continue in loop
                }

                struct pollfd client_poll_arg = {
                    conn->fd,
                    conn->state == STATE_REQ ? POLLIN : POLLOUT,
                    0
                };
                poll_args.push_back(client_poll_arg);
            }
        }

        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), 1000);

        // std::cout << poll_args.size() << std::endl;
        // if (poll_args.size() > 1) {
        //     struct pollfd connection = poll_args.at(1);
        //     std::cout << connection.fd << ' ' << connection.events << ' ' << connection.revents << std::endl;
        //     std::cout << std::endl;
        // }


        // for all active sockets that are ready to be operated on, do the operation specified by conn->state

        for (int i{1}; i < poll_args.size(); i++) {
            struct pollfd connection = poll_args.at(i);
            if (connection.revents) {
                perform_action_on_client(connection.fd, fd2conn);
            }
            std::cout << connection.fd << ' ' << connection.events << ' ' << connection.revents << std::endl;
            std::cout << std::endl;
        }

        if (poll_args[0].revents) {
            int client_fd = accept_new_connection(server_fd, fd2conn);
            resize_connections(client_fd, fd2conn);
            map_new_connection(client_fd, fd2conn);

            // debugging fd2conn
            // for (Conn* conn: fd2conn) {
            //     if (conn) {
            //         std::cout << conn->fd << std::endl;
            //     }
            // }
            // exit(0);
        }
    }
}
