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

/**
 * Create the window data
 *
 * @param id The window id
 * @param title The window title
 * @param app_id The application id
 * @param focused Whether the window is focused
 * @param x Sorting value x (typically window x position)
 * @param y Sorting value y (typically window y position)
 * @return The fully created window data
 */
WindowManagerWindow *wm_win_create(
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int x,
    int y
) {
    WindowManagerWindow *win = g_malloc(sizeof(WindowManagerWindow));

    win->id = g_strdup(id);
    win->title = g_strdup(title);
    win->app_id = g_strdup(app_id);
    win->focused = focused;
    win->x = x;
    win->y = y;

    return win;
}

/**
 * Destroys the window data
 *
 * @param win The window data
 */
void wm_win_destroy(WindowManagerWindow *win) {
    g_free(win->id);
    g_free(win->title);
    g_free(win->app_id);
    g_free(win);
}
