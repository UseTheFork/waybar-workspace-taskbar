#include "common.h"
#include "core/utils.h"
#include "glib.h"
#include <stdio.h>

/**
 * Executes a click action. Builds the command string and sends the command
 *
 * @param format The format string for the command
 * @param id The window id
 * @return TRUE if success else FALSE
 */
gboolean wm_click_execute(const char *format, const char *id) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), format, id);

    cmd_send(cmd);

    return TRUE;
}
