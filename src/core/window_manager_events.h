#pragma once

#include "glib.h"
#include "window_manager.h"
#include <sys/poll.h>

#define WM_EVENTS_MAX_CALlBACKS 8

WindowManagerEvents *window_manager_events_create(
    WindowManagerSocketInit window_manager_socket_init,
    int event_buf_size
);

int window_manager_events_destroy(WindowManagerEvents *events);

int window_manager_events_subscribe(
    WindowManagerEvents *events,
    WindowManagerEventsCallback cb,
    gpointer user_data
);

int window_manager_events_unsubscribe(WindowManagerEvents *events, int pos);
