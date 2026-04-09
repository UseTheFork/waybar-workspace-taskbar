#pragma once

#include "core/app.h"
#include "core/window_manager.h"
#include "glib.h"

G_BEGIN_DECLS;

#define WM_CALLBACK_DEBOUNCE_TIMEOUT 20

typedef struct {
    WwtApp *app;
    WindowManagerEvent *event;
} DebounceCallbackData;

gboolean wm_click_execute(const char *format, const char *id);

G_END_DECLS;
