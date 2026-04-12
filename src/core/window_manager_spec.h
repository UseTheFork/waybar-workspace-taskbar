#pragma once

#include "core/window_manager.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _WindowManagerData WindowManagerData;
typedef struct WindowManagerEvent WindowManagerEvent;

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
typedef WindowManagerData *(*WindowManagerDataGetter)();
typedef gboolean (*WindowManagerClickHandler)(const char *id);
typedef gboolean (*WindowManagerEventsValidator)(WindowManagerEvent *event);

typedef struct WindowManagerSpec {
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsValidator events_validator;
    WindowManagerDataGetter data_getter;
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
WindowManagerDataGetter window_manager_spec_get_data_getter(
    WindowManagerSpec *self
);

WindowManagerSpec *window_manager_spec_create(WindowManagerId wm_id);
void window_manager_spec_destroy(WindowManagerSpec *spec);

G_END_DECLS;
