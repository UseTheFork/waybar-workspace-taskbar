#pragma once

#include "glib.h"
#include <json-glib/json-glib.h>

typedef struct WindowManagerWindow WindowManagerWindow;

struct WindowManagerWindow {
    gchar *id;
    gchar *title;
    gchar *app_id;
    int focused;
    int x;
    int y;
};

gboolean wm_click_execute(const char *format, const char *id);
JsonParser *create_parser(const char *json_str);

WindowManagerWindow *wm_win_create(
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int x,
    int y
);

void wm_win_destroy(WindowManagerWindow *win);
