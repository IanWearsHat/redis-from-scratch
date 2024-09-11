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

const size_t k_max_msg = 4096;
int32_t make_request(int fd, const char *text) {
    uint32_t len = (uint32_t) strlen(text);

    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    int32_t err = write_all(fd, wbuf, 4 + len);
    if (err) {
        if (errno == 0) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cout << "write error" << std::endl;
        }
        return err;
    }

}

int32_t read_response(int fd) {
    // read response
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cout << "read() error" << std::endl;
        }
        return err;
    }

    // Get length of message from header
    uint32_t len;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        std::cout << "too long" << std::endl;
        return -1;
    }

    // Read the number of bytes equal to length of message taken from header
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        std::cout << "read() error" << std::endl;
        return err;
    }

    // Add end of line to buffer so printf can print
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);

    return 0;
}

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    
    std::cout << "Hello\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cout << "Error creating socket";
        return 1;
    }


    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(6379);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    // if (rv) {
    //     std::cout << "Error creating socket";
    //     return 1;
    // }

    // char msg[] = "*1\r\n$4\r\nPING\r\n";

    int fd2 = socket(AF_INET, SOCK_STREAM, 0);
    int rv2 = connect(fd2, (const struct sockaddr *)&addr, sizeof(addr));

    int32_t err;
    err = make_request(fd, "tory lanez");
    err = make_request(fd, "canada");
    err = make_request(fd2, "tight lipped fathers");
    err = make_request(fd, "just joshin");

    read_response(fd2);
    read_response(fd);
    read_response(fd);
    read_response(fd);

    close(fd);
    close(fd2);

    return 0;
}