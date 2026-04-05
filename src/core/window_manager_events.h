#pragma once

#include "glib.h"
#include "window_manager.h"
#include <sys/poll.h>

#define WM_EVENTS_MAX_CALlBACKS 8

WindowManagerEvents *window_manager_events_create(
    WindowManagerEventsConstructor events_constructor,
    WindowManagerEventsReader events_reader
);

void window_manager_events_destroy(
    WindowManagerEvents *events,
    WindowManagerEventsDestructor destructor
);

int window_manager_events_subscribe(
    WindowManagerEvents *events,
    WindowManagerEventsCallback cb,
    gpointer user_data
);

int window_manager_events_unsubscribe(WindowManagerEvents *events, int pos);
