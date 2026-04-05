#include "utils.h"
#include "glib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/**
 * Send a command no output
 *
 * @param cmd The command to send
 * @return The return value of system()
 */
int cmd_send(const char *cmd) {
    return system(cmd);
}

/**
 * Send a command and get the output
 *
 * @param cmd The command to send
 * @return (transfer full): The output
 */
char *cmd_output(const char *cmd) {
    FILE *fp = popen(cmd, "r");

    if (!fp) {
        return NULL;
    }

    size_t size = 4096;
    char *buf = g_malloc(size);
    size_t len = 0;
    char tmp[256];

    while (fgets(tmp, sizeof(tmp), fp)) {
        size_t chunk = strlen(tmp);

        if (len + chunk + 1 > size) {
            size *= 2;
            buf = g_realloc(buf, size);
        }

        memcpy(buf + len, tmp, chunk);
        len += chunk;
    }

    buf[len] = '\0';
    pclose(fp);

    return buf;
}

/**
 * Connect to a socket
 *
 * @param socket_path The address path to connect to
 * @return The socket file descriptor, or -1 on failure
 */
int socket_connect(const char *socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * Creates the json parser
 *
 * @param json_str The string you want the parser to populate
 * @return The parser
 */
JsonParser *create_json_parser(const char *json_str) {
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    json_parser_load_from_data(parser, json_str, -1, &error);

    if (error) {
        g_warning("Parse error: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);

        return NULL;
    }

    return parser;
}
