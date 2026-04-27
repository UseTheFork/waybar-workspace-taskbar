#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _WindowManagerData WindowManagerData;
typedef struct WindowManagerEvent WindowManagerEvent;

typedef enum WindowManagerId {
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

typedef int (*WindowManagerEventsConstructor)();
typedef void (*WindowManagerEventsDestructor)(int fd, FILE *socket_file);
typedef gboolean (*WindowManagerEventsReader)(
    FILE *socket_file,
    WindowManagerEvent *event
);
typedef WindowManagerData *(*WindowManagerDataFetcher)();
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef gboolean (*WindowManagerEventsValidator)(WindowManagerEvent *event);

typedef struct WindowManagerSpec {
    WindowManagerId id;
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsValidator events_validator;
    WindowManagerDataFetcher data_fetcher;
    WindowManagerClickHandler window_focus;
    WindowManagerClickHandler window_close;
    WindowManagerClickHandler window_float;
} WindowManagerSpec;

WindowManagerEventsConstructor window_manager_spec_get_events_constructor(
    WindowManagerSpec *self
);
WindowManagerEventsDestructor window_manager_spec_get_events_destructor(
    WindowManagerSpec *self
);
WindowManagerEventsReader window_manager_spec_get_events_reader(
    WindowManagerSpec *self
);
WindowManagerEventsValidator window_manager_spec_get_events_validator(
    WindowManagerSpec *self
);
WindowManagerClickHandler window_manager_spec_get_click_handler(
    WindowManagerSpec *self,
    WindowManagerClickHandlerType type
);
WindowManagerDataFetcher window_manager_spec_get_data_fetcher(
    WindowManagerSpec *self
);

WindowManagerSpec *window_manager_spec_create(WindowManagerId wm_id);
void window_manager_spec_destroy(WindowManagerSpec *spec);

G_END_DECLS;
