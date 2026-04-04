#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_WINDOW_MANAGER_TYPE (wwt_window_manager_get_type())

G_DECLARE_FINAL_TYPE(
    WwtWindowManager,
    wwt_window_manager,
    WWT,
    WINDOW_MANAGER,
    GObject
)

typedef struct _WwtApp WwtApp;
typedef struct _WwtWindowManager WwtWindowManager;
typedef struct _WindowManagerEvents WindowManagerEvents;
typedef struct _WindowManagerEventsSubscription WindowManagerEventsSubscription;

typedef enum { WM_ID_SWAY, WM_ID_NIRI, WM_ID_HYPRLAND } WindowManagerId;
typedef enum {
    WM_CLICK_FOCUS,
    WM_CLICK_FLOAT,
    WM_CLICK_CLOSE
} WindowManagerClickHandlerType;

typedef int (*WindowManagerSocketInit)();
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef gboolean (*WindowManagerGetWindows)(WwtApp *app, GPtrArray *tabs);
typedef void (*WindowManagerEventsCallback)(
    const char *event,
    gpointer user_data
);

typedef struct WindowManagerSpec {
    int events_buf_size;
    WindowManagerSocketInit socket_init;
    WindowManagerEventsCallback events_callback;
    WindowManagerGetWindows get_windows;
    WindowManagerClickHandler window_focus;
    WindowManagerClickHandler window_close;
    WindowManagerClickHandler window_float;
} WindowManagerSpec;

WwtWindowManager *window_manager_new(WwtApp *app, WindowManagerId id);

WindowManagerClickHandler wwt_window_manager_get_click_handler(
    WwtWindowManager *self,
    WindowManagerClickHandlerType type
);

WindowManagerGetWindows wwt_window_manager_get_get_windows(
    WwtWindowManager *self
);

G_END_DECLS;
