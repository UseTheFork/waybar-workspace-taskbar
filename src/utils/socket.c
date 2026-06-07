#include "socket.h"
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_READ_CHUNK 4096

/**
 * Connect to a socket
 *
 * @param socket_path The address path to connect to
 * @return The socket file descriptor, or -1 on failure
 */
int socket_connect(const char *socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(fd < 0) {
        perror("Failed socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Failed socket connect");
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * Reads the socket to the max amount or end of file.
 *
 * Call this if you don't already know the amount of characters you want to
 * read.
 *
 * @param fd The socket file descriptor
 * @param max The max amount of bytes to read
 * @return (transfer full) A string read from the socket
 */
char *socket_read(int fd, size_t max) {
    size_t read_max = max ? max : SOCKET_READ_MAX;
    size_t buf_size = SOCKET_READ_CHUNK;
    size_t bytes_total = 0;
    ssize_t bytes_read;

    char *buf = g_malloc(buf_size + 1);

    if(!buf) {
        return NULL;
    }

    while(1) {
        if(bytes_total + SOCKET_READ_CHUNK > buf_size) {
            buf_size *= 2;
            char *tmp = g_realloc(buf, buf_size + 1);

            if(!tmp) {
                g_free(buf);
                return NULL;
            }

            buf = tmp;
        }

        bytes_read = read(fd, buf + bytes_total, buf_size - bytes_total);

        if(bytes_read <= 0) {
            break;
        }

        bytes_total += bytes_read;

        if(bytes_total >= read_max) {
            break;
        }
    }

    buf[bytes_total] = '\0';

    return buf;
}

/**
 * Reads from the socket the specified length
 *
 * Call this if you know the amount of characters you want to read
 *
 * @param fd The socket file descriptor
 * @param len The amount of bytes to read
 * @return (transfer full): A string of n chars read from the socket
 */
char *socket_read_len(int fd, size_t len) {
    char *buf = g_malloc(len + 1);
    if(!buf) {
        return NULL;
    }

    size_t bytes_total = 0;

    while(bytes_total < len) {
        ssize_t bytes_read = read(fd, buf + bytes_total, len - bytes_total);

        if(bytes_read <= 0) {
            break;
        }

        bytes_total += bytes_read;
    }

    buf[bytes_total] = '\0';

    return buf;
}
