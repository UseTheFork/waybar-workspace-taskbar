#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WM_EVENTS_MAX_CALlBACKS 8

typedef struct _WindowManagerData WindowManagerData;
typedef struct _WindowManagerEvents WindowManagerEvents;
typedef struct WindowManagerSpec WindowManagerSpec;

typedef struct WindowManagerEvent {
    char *msg;
    size_t msg_len;
    size_t msg_size;
    gint debounce_timeout_id;
} WindowManagerEvent;

typedef void (*WindowManagerEventsCallback)(
    WindowManagerData *wm_data,
    gpointer user_data
);

typedef struct WindowManagerEventsSubscription {
    WindowManagerEventsCallback cb;
    gpointer user_data;
} WindowManagerEventsSubscription;

WindowManagerEvents *window_manager_events_create(WindowManagerSpec *spec);
void window_manager_events_destroy(WindowManagerEvents *events);

int window_manager_events_subscribe(
    WindowManagerEvents *events,
    WindowManagerEventsCallback cb,
    gpointer user_data
);

int window_manager_events_unsubscribe(WindowManagerEvents *events, int pos);

G_END_DECLS;
