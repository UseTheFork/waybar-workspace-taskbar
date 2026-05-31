#include "events.h"
#include "glib.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

#define WM_EVENTS_POLLING_TIMEOUT 20
#define WM_EVENTS_DEBOUNCE_TIMEOUT 20
#define WM_EVENTS_MSG_SIZE 4096

struct _WwtWindowManagerEvents {
    GObject parent_instance;

    int timeout_id;
    int socket_fd;
    FILE *socket_file;
    struct pollfd poll_fd;
    WindowManagerEvent *event;
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsValidator events_validator;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsCallback events_callback;
    gpointer events_callback_user_data;
};

G_DEFINE_TYPE(WwtWindowManagerEvents, wwt_window_manager_events, G_TYPE_OBJECT);

/**
 * Sets non blocking on the socket file
 *
 * @param fd Socket file descriptor
 */
static void set_non_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * Creates the window manager event
 *
 * Uses malloc instead of g_malloc because readline will use stdlib to resize
 *
 * @return The fully created window manager event
 */
static WindowManagerEvent *window_manager_event_create() {
    char *msg = malloc(WM_EVENTS_MSG_SIZE);
    WindowManagerEvent *event = malloc(sizeof(WindowManagerEvent));
    event->msg = msg;
    event->msg_len = 0;
    event->msg_size = WM_EVENTS_MSG_SIZE;
    event->debounce_timeout_id = 0;

    return event;
}

/**
 * Destroys the window manager events
 *
 * Again use free instead of g_free because we use stdlib to create so readline
 * can correctly resize the msg
 *
 * @param event
 */
static void window_manager_event_destroy(WindowManagerEvent *event) {
    if(event->debounce_timeout_id) {
        g_source_remove(event->debounce_timeout_id);
    }

    free(event->msg);
    free(event);
}

/**
 * Function to call after debounce timeout success
 *
 * @param user_data The data to be passed (self) in this case
 */
static gboolean events_debounce_fn(gpointer user_data) {
    WwtWindowManagerEvents *self = user_data;

    self->events_callback(self->events_callback_user_data);
    self->event->debounce_timeout_id = 0;

    return G_SOURCE_REMOVE;
}

/**
 * Poll for socket events
 *
 * @param user_data In this case the events instance
 * @return TRUE if no error else FALSE
 */
static gboolean window_manager_events_poll(gpointer user_data) {
    WwtWindowManagerEvents *self = user_data;

    int ret = poll(&self->poll_fd, 1, 0);

    if(ret < 0) {
        return FALSE;
    }

    if(self->poll_fd.revents & (POLLERR | POLLHUP)) {
        return FALSE;
    }

    if(self->poll_fd.revents & POLLIN) {
        while(self->events_reader(self->socket_file, self->event)) {
            if(!self->events_validator(self->event)) {
                continue;
            }

            if(self->event->debounce_timeout_id != 0) {
                g_source_remove(self->event->debounce_timeout_id);
                self->event->debounce_timeout_id = 0;
            }

            self->event->debounce_timeout_id = g_timeout_add(
                WM_EVENTS_DEBOUNCE_TIMEOUT,
                events_debounce_fn,
                self
            );
        }
    }

    return TRUE;
}

/**
 * Destroy the window manager events
 *
 * @pararm events
 * @param destructor The window manager specific events destructor
 */
static void finalize(GObject *obj) {
    WwtWindowManagerEvents *self = WWT_WINDOW_MANAGER_EVENTS(obj);

    if(!self) {
        return;
    }

    self->events_destructor(self->poll_fd.fd, self->socket_file);
    window_manager_event_destroy(self->event);
    g_source_remove(self->timeout_id);

    G_OBJECT_CLASS(wwt_window_manager_events_parent_class)->finalize(obj);
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_window_manager_events_init(WwtWindowManagerEvents *self) {
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_window_manager_events_class_init(
    WwtWindowManagerEventsClass *klass
) {
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Create the window manager events
 *
 * @param spec The window manager spec
 * @return (transfer full): The the events instance
 */
WwtWindowManagerEvents *wwt_window_manager_events_new(
    WindowManagerEventsConstructor events_constructor,
    WindowManagerEventsDestructor events_destructor,
    WindowManagerEventsReader events_reader,
    WindowManagerEventsValidator events_validator,
    WindowManagerEventsCallback events_callback,
    gpointer events_callback_user_data
) {
    WwtWindowManagerEvents *self =
        g_object_new(WWT_WINDOW_MANAGER_EVENTS_TYPE, NULL);

    self->events_constructor = events_constructor;
    self->events_destructor = events_destructor;
    self->events_reader = events_reader;
    self->events_validator = events_validator;
    self->events_callback = events_callback;
    self->events_callback_user_data = events_callback_user_data;

    int fd = self->events_constructor();

    self->socket_file = fdopen(fd, "r");

    if(!self->socket_file) {
        g_free(self);
        perror("fdopen");
        close(fd);
        return NULL;
    }

    self->event = window_manager_event_create();
    self->poll_fd.fd = fd;
    self->poll_fd.events = POLLIN;
    self->timeout_id = g_timeout_add(
        WM_EVENTS_POLLING_TIMEOUT,
        window_manager_events_poll,
        self
    );

    set_non_block(self->poll_fd.fd);

    return self;
}
