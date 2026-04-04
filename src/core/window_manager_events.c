#include "window_manager_events.h"
#include "glib.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

#define WM_EVENTS_CALLBACK_TIMEOUT 50

struct _WindowManagerEvents {
    FILE *socket_file;
    int socket_fd;
    struct pollfd poll_fd;
    int event_buf_size;
    int timeout_id;
    WindowManagerEventsSubscription *subs[WM_EVENTS_MAX_CALlBACKS];
};

struct _WindowManagerEventsSubscription {
    WindowManagerEventsCallback cb;
    gpointer user_data;
};

static void set_non_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static gboolean window_manager_events_poll(gpointer user_data) {
    WindowManagerEvents *events = user_data;
    int ret = poll(&events->poll_fd, 1, 0);

    if (ret < 0) {
        return FALSE;
    }

    if (events->poll_fd.revents & (POLLERR | POLLHUP)) {
        return FALSE;
    }

    if (events->poll_fd.revents & POLLIN) {
        char buf[events->event_buf_size];

        while (fgets(buf, sizeof(buf), events->socket_file) != NULL) {
            for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
                if (events->subs[i]) {
                    events->subs[i]->cb(buf, events->subs[i]->user_data);
                }
            }
        }
    }

    return TRUE;
}

WindowManagerEvents *window_manager_events_create(
    WindowManagerSocketInit window_manager_socket_init,
    int event_buf_size
) {
    WindowManagerEvents *events = g_malloc0(sizeof(WindowManagerEvents));

    int fd = window_manager_socket_init();

    events->socket_file = fdopen(fd, "r");

    if (!events->socket_file) {
        g_free(events);
        perror("fdopen");
        close(fd);
        return NULL;
    }

    events->event_buf_size = event_buf_size;
    events->poll_fd.fd = fd;
    events->poll_fd.events = POLLIN;
    events->event_buf_size = event_buf_size;
    events->timeout_id = g_timeout_add(
        WM_EVENTS_CALLBACK_TIMEOUT,
        window_manager_events_poll,
        events
    );

    set_non_block(events->poll_fd.fd);

    return events;
}

gboolean window_manager_events_destroy(WindowManagerEvents *events) {
    fclose(events->socket_file);

    for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
        if (!events->subs[i]) {
            window_manager_events_unsubscribe(events, i);
        }
    }

    g_source_remove(events->timeout_id);
    g_free(events);

    return TRUE;
}

int window_manager_events_subscribe(
    WindowManagerEvents *events,
    WindowManagerEventsCallback cb,
    gpointer user_data
) {
    for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
        if (!events->subs[i]) {
            WindowManagerEventsSubscription *sub =
                g_malloc(sizeof(WindowManagerEventsSubscription));

            sub->cb = cb;
            sub->user_data = user_data;
            events->subs[i] = sub;
            return i;
        }
    }
    return -1;
}

gboolean window_manager_events_unsubscribe(
    WindowManagerEvents *events,
    int pos
) {
    if (pos < 0 || pos >= WM_EVENTS_MAX_CALlBACKS) {
        return FALSE;
    }

    if (events->subs[pos]) {
        g_free(events->subs[pos]);
        events->subs[pos] = NULL;
    }

    return TRUE;
}
