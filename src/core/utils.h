#pragma once

#include <json-glib/json-glib.h>

G_BEGIN_DECLS

int cmd_send(const char *cmd);
char *cmd_output(const char *cmd);
int socket_connect(const char *socket_path);
JsonParser *create_json_parser(const char *json_str);
long long get_timestamp();

G_END_DECLS
