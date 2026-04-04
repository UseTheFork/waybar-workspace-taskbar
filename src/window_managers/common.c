#include "common.h"
#include "core/utils.h"
#include "glib.h"
#include <stdio.h>

gboolean wm_click_execute(const char *format, const char *id) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), format, id);

    cmd_send(cmd);

    return TRUE;
}

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

void wm_win_destroy(WindowManagerWindow *win) {
    g_free(win->id);
    g_free(win->title);
    g_free(win->app_id);
    g_free(win);
}

JsonParser *create_parser(const char *json_str) {
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
