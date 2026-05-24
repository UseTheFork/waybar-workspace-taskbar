#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

int cmd_run(const char *cmd);
char *cmd_run_output(const char *cmd);
int cmd_run_fork_exec(const char *cmd);

G_END_DECLS
