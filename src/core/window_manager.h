#pragma once

#include "window_manager_data.h"
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
typedef struct _WwtWindowManagerWorkspaces WwtWindowManagerWorkspaces;

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
    size_t msg_len;
    size_t msg_size;
    gint debounce_timeout_id;
} WindowManagerEvent;

typedef int (*WindowManagerEventsConstructor)();
typedef void (*WindowManagerEventsDestructor)(int fd, FILE *socket_file);
typedef gboolean (*WindowManagerEventsValidator)(WindowManagerEvent *event);
typedef gboolean (*WindowManagerEventsReader)(
    FILE *socket_file,
    WindowManagerEvent *event
);
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef WindowManagerData *(*WindowManagerGetData)();

typedef void (*WindowManagerEventsSubscriptionCallback)(
    WindowManagerData *wm_data,
    gpointer user_data
);

typedef struct WindowManagerEventsSubscription {
    WindowManagerEventsSubscriptionCallback cb;
    gpointer user_data;
} WindowManagerEventsSubscription;

typedef struct WindowManagerSpec {
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsValidator events_validator;
    WindowManagerGetData get_data;
    WindowManagerClickHandler window_focus;
    WindowManagerClickHandler window_close;
    WindowManagerClickHandler window_float;
} WindowManagerSpec;

WwtWindowManager *window_manager_default(WwtApp *app, WindowManagerId id);

WindowManagerClickHandler wwt_window_manager_get_click_handler(
    WwtWindowManager *self,
    WindowManagerClickHandlerType type
);

WwtWindowManagerWorkspaces *wwt_window_manager_get_workspaces(
    WwtWindowManager *self
);

WindowManagerGetData wwt_window_manager_get_get_data(WwtWindowManager *self);

int wwt_window_manager_events_subscribe(
    WwtWindowManager *self,
    WindowManagerEventsSubscriptionCallback cb,
    gpointer user_data
);

gboolean wwt_window_manager_events_unsubscribe(WwtWindowManager *self, int id);

G_END_DECLS;
