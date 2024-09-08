#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

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

    for (int i = 0; i < 2; i++) {
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(6379);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
        int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
        // if (rv) {
        //     std::cout << "Error creating socket";
        //     return 1;
        // }

        char msg[] = "hello";
        write(fd, msg, strlen(msg));

        char rbuf[64] = {};
        ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
        if (n < 0) {
            std::cout << "Error reading response";
        }
        printf("server says: %s\n", rbuf);
    }
    
    close(fd);

    return 0;
}