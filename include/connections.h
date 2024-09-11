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
    uint8_t* last_request_end_pointer = rbuf;

    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

static int32_t read_full(int fd, uint8_t *buf, size_t n);

static int32_t write_all(int fd, const uint8_t *buf, size_t n);

void die(const char* msg);

bool try_process_one_request(Conn *client);

bool fill_read_buffer(Conn *client);

void handle_request_state(Conn *client);

bool try_flush_buffer(Conn *client);

void handle_response_state(Conn *client);

void perform_action_on_client(int client_fd, std::vector<Conn *> &fd2conn);

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

/**
 * Flow of loop
 * 
 * If connection is marked for end
 *      end connection
 * 
 * If connection has request:
 *      while (true)
    *      Fill read buffer with data from connection
    * 
    *      if reading causes error && errno == EAGAIN:
    *          simply stop, but dont mark connection for end
    *      if errno != EAGAIN:
    *          mark connection for end
    *           break out of while(true)
    *      if reading == 0, then we reached EOF:
    *          mark connection for end
    *           break out of while(true)
    * 
    *      while read buffer is not empty:
    *          try to process one request from buffer
    *          if successful,
    *               remove request from buffer
    *               write_response to write buffer
    *               flush_write_buffer
    *               change state back to REQ because that means we successfully wrote a response, but there might be more requests to handle
    *          if unsuccessful,
    *              if the len + 4 > length of data currently in buffer, it means we have an incomplete request by nature of us trying to fill the entire buffer
    *                  therefore we run outer while (true) loop again
    *                   ig here we just break out of inner loop
    *              if error:
    *                  mark connection for end
    *                  break out of while(true)
 * 
 * If connection has response:
 *      write_response to write buffer
 * 
 */