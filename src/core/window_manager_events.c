#include "window_manager_events.h"
#include "glib.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

#define WM_EVENTS_CALLBACK_TIMEOUT 50
#define WM_EVENTS_MSG_SIZE 4096

struct _WindowManagerEvents {
    int timeout_id;
    int socket_fd;
    FILE *socket_file;
    struct pollfd poll_fd;
    WindowManagerEvent *event;
    WindowManagerEventsReader reader;
    WindowManagerEventsSubscription *subs[WM_EVENTS_MAX_CALlBACKS];
};

struct _WindowManagerEventsSubscription {
    WindowManagerEventsCallback cb;
    gpointer user_data;
};

/**
 * Creates the window manager event
 *
 * Uses malloc instead of g_malloc because readline will use stdlib to resize
 */
static WindowManagerEvent *window_manager_event_create() {
    char *msg = malloc(WM_EVENTS_MSG_SIZE);
    WindowManagerEvent *event = malloc(sizeof(WindowManagerEvent));
    event->msg = msg;
    event->msg_len = 0;
    event->msg_size = WM_EVENTS_MSG_SIZE;

    return event;
}

/**
 * Destroys the window manager event
 *
 * Again use free instead of g_free because we use stdlib to create so readline
 * can correctly resize the msg
 *
 * @param event
 */
static void window_manager_event_destroy(WindowManagerEvent *event) {
    free(event->msg);
    free(event);
}

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

        while (events->reader(events->socket_file, events->event)) {
            for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
                if (events->subs[i]) {
                    events->subs[i]->cb(
                        events->event,
                        events->subs[i]->user_data
                    );
                }
            }
        }

        // while (fgets(buf, sizeof(buf), events->socket_file) != NULL) {
        //     for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
        //         if (events->subs[i]) {
        //             events->subs[i]->cb(buf, events->subs[i]->user_data);
        //         }
        //     }
        // }
    }

    return TRUE;
}

WindowManagerEvents *window_manager_events_create(
    WindowManagerEventsConstructor events_constructor,
    WindowManagerEventsReader events_reader
) {
    WindowManagerEvents *events = g_malloc0(sizeof(WindowManagerEvents));

    int fd = events_constructor();

    events->socket_file = fdopen(fd, "r");

    if (!events->socket_file) {
        g_free(events);
        perror("fdopen");
        close(fd);
        return NULL;
    }

    events->event = window_manager_event_create();
    events->reader = events_reader;
    events->poll_fd.fd = fd;
    events->poll_fd.events = POLLIN;
    events->timeout_id = g_timeout_add(
        WM_EVENTS_CALLBACK_TIMEOUT,
        window_manager_events_poll,
        events
    );

    set_non_block(events->poll_fd.fd);

    return events;
}

gboolean window_manager_events_destroy(
    WindowManagerEvents *events,
    WindowManagerEventsDestructor destructor
) {
    for (int i = 0; i < WM_EVENTS_MAX_CALlBACKS; ++i) {
        if (events->subs[i]) {
            window_manager_events_unsubscribe(events, i);
        }
    }

    // fclose(events->socket_file);
    destructor(events->poll_fd.fd, events->socket_file);

    window_manager_event_destroy(events->event);
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
