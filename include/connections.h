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

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,  // mark the connection for deletion
};

const size_t k_max_msg = 4096;

struct Conn {
    int fd = -1;
    uint32_t state = STATE_REQ;     // either STATE_REQ or STATE_RES

    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];

    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

void die(const char* msg);

void map_new_connection(int client_fd, std::vector<Conn *> &fd2conn);

void resize_connections(int client_fd, std::vector<Conn *> &fd2conn);

int accept_new_connection(int server_fd, std::vector<Conn *> &fd2conn);

void run_event_loop(int fd);
/**
 * all_fds = [...]
    while True:
        active_fds = poll(all_fds)
        for each fd in active_fds:
            do_something_with(fd)

    def do_something_with(fd):
        if fd is a listening socket:
            add_new_client(fd)
        elif fd is a client connection:
            while work_not_done(fd):
                do_something_to_client(fd)

    def do_something_to_client(fd):
        if should_read_from(fd):
            data = read_until_EAGAIN(fd)
            process_incoming_data(data)
        while should_write_to(fd):
            write_until_EAGAIN(fd)
        if should_close(fd):
            destroy_client(fd)
 */