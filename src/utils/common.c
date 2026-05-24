#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

/**
 * Connect to a socket
 *
 * @param socket_path The address path to connect to
 * @return The socket file descriptor, or -1 on failure
 */
int socket_connect(const char *socket_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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

    if(error) {
        g_warning("Parse error: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);

        return NULL;
    }

    return parser;
}

/**
 * Gets the current timestamp in ms
 *
 * @return The timestamp in ms
 */
long long get_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long long ms = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    return ms;
}

/**
 * Creates a new string and replaces the characters specified
 *
 * @param str The string you want to be replaced
 * @param find The characters to find
 * @param replace The characters you want to replace with
 * @return (transfer full) A new string with the characters replaced (caller
 * frees using g_free())
 */
gchar *str_replace(const char *str, const char *find, const char *replace) {
    GString *g_str = g_string_new(str);
    g_string_replace(g_str, find, replace, 0);

    return g_string_free(g_str, FALSE);
}
