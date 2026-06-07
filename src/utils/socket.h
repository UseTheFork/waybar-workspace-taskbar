#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SOCKET_READ_MAX (256 * 1024) // 256k

int socket_connect(const char *socket_path);
char *socket_read(int fd, size_t max);
char *socket_read_len(int fd, size_t len);

G_END_DECLS
