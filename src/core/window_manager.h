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

typedef enum {
    WM_ID_UNSUPPORTED,
    WM_ID_SWAY,
    WM_ID_NIRI,
    WM_ID_HYPRLAND
} WindowManagerId;

typedef enum {
    WM_CLICK_FOCUS,
    WM_CLICK_FLOAT,
    WM_CLICK_CLOSE
} WindowManagerClickHandlerType;

typedef struct {
    char *msg;
    int msg_len;
    int msg_size;
} WindowManagerEvent;

typedef int (*WindowManagerEventsConstructor)();
typedef void (*WindowManagerEventsDestructor)(int fd, FILE *socket_file);
typedef gboolean (*WindowManagerEventsReader)(
    FILE *socket_file,
    WindowManagerEvent *event
);
typedef void (*WindowManagerEventsCallback)(
    WindowManagerEvent *event,
    gpointer user_data
);
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef gboolean (*WindowManagerGetWindows)(WwtApp *app, GPtrArray *tabs);

typedef struct WindowManagerSpec {
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
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
