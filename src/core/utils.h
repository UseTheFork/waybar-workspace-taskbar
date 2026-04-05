#pragma once

#include <json-glib/json-glib.h>

int cmd_send(const char *cmd);
char *cmd_output(const char *cmd);

int socket_connect(const char *socket_path);
JsonParser *create_json_parser(const char *json_str);
