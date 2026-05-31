#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _WwtWindowManagerEvents WwtWindowManagerEvents;
typedef struct _WwtWindowManagerSubscriptions WwtWindowManagerSubscriptions;
typedef struct _WwtWindowManagerData WwtWindowManagerData;
typedef struct WindowManagerEvent WindowManagerEvent;

#define WWT_WINDOW_MANAGER_TYPE (wwt_window_manager_get_type())

G_DECLARE_FINAL_TYPE(
    WwtWindowManager,
    wwt_window_manager,
    WWT,
    WINDOW_MANAGER,
    GObject
);

typedef enum WindowManagerId {
    WM_ID_UNSUPPORTED,
    WM_ID_SWAY,
    WM_ID_NIRI,
    WM_ID_HYPRLAND
} WindowManagerId;

typedef void (*WindowManagerEventsCallback)(gpointer user_data);
typedef int (*WindowManagerEventsConstructor)();
typedef void (*WindowManagerEventsDestructor)(int fd, FILE *socket_file);
typedef gboolean (*WindowManagerEventsReader)(
    FILE *socket_file,
    WindowManagerEvent *event
);
typedef void (*WindowManagerDataFetcher)(WwtWindowManagerData *wm_data);
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef gboolean (*WindowManagerEventsValidator)(WindowManagerEvent *event);

typedef struct {
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsValidator events_validator;
    WindowManagerDataFetcher data_fetcher;
} WindowManagerSpecFactory;

WwtWindowManager *wwt_window_manager_new(WindowManagerId);
WindowManagerId wwt_window_manager_get_id(WwtWindowManager *self);
WwtWindowManagerSubscriptions *wwt_window_manager_get_subsciptions(
    WwtWindowManager *self
);
WwtWindowManagerData *wwt_window_manager_get_data(WwtWindowManager *self);

G_END_DECLS
