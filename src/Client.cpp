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

void write_text_to_buffer(char *buf, const std::vector<std::string> &args) {
    int pos{0};
    for (const std::string str: args) {
        int len = str.size();
        memcpy(&buf[pos], &len, 4);
        strcpy(&buf[pos + 4], str.c_str());

        printf("bruh %s\n", &buf[pos + 4]);

        pos += 4 + len;
    }
}

int get_request_length(const std::vector<std::string> &args) {
    int total{8};
    for (const std::string str: args) {
        total += 4 + str.size();
    }
    return total;
}

const size_t k_max_msg = 4096;
int32_t make_request(int fd, const std::vector<std::string> &args) {
    int num_args = args.size();
    int request_byte_length = get_request_length(args);

    char wbuf[8 + request_byte_length];
    memcpy(wbuf, &request_byte_length, 4);
    memcpy(&wbuf[4], &num_args, 4);

    write_text_to_buffer(&wbuf[8], args);

    int test1;
    memcpy(&test1, wbuf, 4);

    int test2;
    memcpy(&test2, &wbuf[4], 4);

    int test3;
    memcpy(&test3, &wbuf[8], 4);

    char test4[test3 + 1];
    memcpy(&test4, &wbuf[8 + 4], test3);
    test4[test3] = '\0';

    int test5;
    memcpy(&test5, &wbuf[8 + 4 + test3], 4);

    char test6[test5 + 1];
    memcpy(&test6, &wbuf[8 + 4 + 4 + test3], test5);
    test6[test5] = '\0';

    std::cout << "Request Length: " << test1 << std::endl;
    std::cout << "Word count: " << test2 << std::endl;
    std::cout << "First word length: " << test3 << std::endl;
    std::cout << "Second word length: " << test5 << std::endl;

    printf("First word: %s\n", test4);
    printf("Second word: %s\n", test6);

    int32_t err = write_all(fd, wbuf, 4 + request_byte_length);
    if (err) {
        if (errno == 0) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cout << "write error" << std::endl;
        }
        return err;
    }

    return 0;
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

    std::vector<std::string> request = {"tory", "lanez", "canada"};

    int32_t err;
    err = make_request(fd, request);
    // err = make_request(fd, "canada");
    // err = make_request(fd2, "tight lipped fathers");
    // err = make_request(fd, "just joshin");

    // read_response(fd2);
    // read_response(fd);
    // read_response(fd);
    read_response(fd);

    close(fd);
    close(fd2);

    return 0;
}