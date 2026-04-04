#pragma once

int cmd_send(const char *cmd);
char *cmd_output(const char *cmd);

int socket_connect(const char *socket_path);
