#pragma once

#include "core/app.h"
#include "core/window_manager.h"
#include "glib.h"

#define WM_CALLBACK_DEBOUNCE_TIMEOUT 20

typedef struct WindowManagerWindow WindowManagerWindow;

typedef struct {
    WwtApp *app;
    WindowManagerEvent *event;
} DebounceCallbackData;

struct WindowManagerWindow {
    gchar *id;
    gchar *title;
    gchar *app_id;
    int focused;
    int x;
    int y;
};

gboolean wm_click_execute(const char *format, const char *id);

WindowManagerWindow *wm_win_create(
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int x,
    int y
);

void wm_win_destroy(WindowManagerWindow *win);
