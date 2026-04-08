#include "window_manager.h"
#include "window_managers/hyprland.h"
#include "window_managers/niri.h"
#include "window_managers/sway.h"
#include <fcntl.h>
#include <sys/poll.h>

#define WM_EVENTS_POLLING_TIMEOUT 20
#define WM_EVENTS_MSG_SIZE 4096

typedef struct {
    int timeout_id;
    int socket_fd;
    FILE *socket_file;
    struct pollfd poll_fd;
} WindowManagerEventsPollData;

struct _WwtWindowManager {
    GObject parent;

    WwtApp *app;
    WindowManagerId id;
    WindowManagerSpec *spec;
    WindowManagerEvent *event;
    WindowManagerEventsPollData *poll_data;
};

G_DEFINE_TYPE(WwtWindowManager, wwt_window_manager, G_TYPE_OBJECT);

/**
 * Gets the generate tabs function from the window manager spec
 *
 * @param self The window manager struct
 * @return The get_windows function from window manager spec
 */
WindowManagerGetWindows wwt_window_manager_get_get_windows(
    WwtWindowManager *self
) {
    return self->spec->get_windows;
}

/**
 * Gets the click handler from the the window manager based on type
 *
 * @param self Window manager struct
 * @param type The click handler type
 * @return The click handler from window manager spec
 */
WindowManagerClickHandler wwt_window_manager_get_click_handler(
    WwtWindowManager *self,
    WindowManagerClickHandlerType type
) {

    if (type == WM_CLICK_FOCUS) {
        return self->spec->window_focus;
    }

    if (type == WM_CLICK_CLOSE) {
        return self->spec->window_close;
    }

    if (type == WM_CLICK_FLOAT) {
        return self->spec->window_float;
    }

    return NULL;
}

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
 * Poll for socket events
 *
 * @param user_data In this case the events instance
 * @return TRUE if no error else FALSE
 */
static gboolean window_manager_events_poll(gpointer user_data) {
    WwtWindowManager *wm = user_data;
    WindowManagerEventsPollData *poll_data = wm->poll_data;
    int ret = poll(&poll_data->poll_fd, 1, 0);

    if (ret < 0) {
        return FALSE;
    }

    if (poll_data->poll_fd.revents & (POLLERR | POLLHUP)) {
        return FALSE;
    }

    if (poll_data->poll_fd.revents & POLLIN) {
        while (wm->spec->events_reader(poll_data->socket_file, wm->event)) {
            wm->spec->events_callback(wm->event, wm->app);
        }
    }

    return TRUE;
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
    if (event->debounce_timeout_id) {
        g_source_remove(event->debounce_timeout_id);
    }

    free(event->msg);
    free(event);
}

/**
 * Create the poll data for window manager events
 *
 * @param self
 */
static WindowManagerEventsPollData *window_manager_events_poll_create(
    WwtWindowManager *self
) {
    WindowManagerEventsPollData *poll_data =
        g_malloc0(sizeof(WindowManagerEventsPollData));

    int fd = self->spec->events_constructor();
    poll_data->poll_fd.fd = fd;
    poll_data->poll_fd.events = POLLIN;
    poll_data->socket_file = fdopen(fd, "r");

    if (!poll_data->socket_file) {
        g_free(poll_data);
        perror("fdopen");
        close(fd);

        return NULL;
    }

    poll_data->timeout_id = g_timeout_add(
        WM_EVENTS_POLLING_TIMEOUT,
        window_manager_events_poll,
        self
    );

    set_non_block(poll_data->poll_fd.fd);

    return poll_data;
}

/**
 * Destroys the poll data for window manager events
 *
 * @param poll_data The poll data struct
 * @param events_destructor The events destructor from the window manager spec
 */
static void window_manager_events_poll_destroy(
    WindowManagerEventsPollData *poll_data,
    WindowManagerEventsDestructor events_destructor
) {
    events_destructor(poll_data->poll_fd.fd, poll_data->socket_file);
    g_source_remove(poll_data->timeout_id);
    g_free(poll_data);
}

/**
 * Initiaizes the window manager spec
 *
 * @param self
 * @param wm_id The window manager id
 * @return TRUE if sucessfully initialized else FALSE
 */
static gboolean wwt_window_manager_init_spec(
    WwtWindowManager *self,
    WindowManagerId wm_id
) {
    if (wm_id == WM_ID_NIRI) {
        self->spec = window_manager_spec_create_niri();

        return TRUE;
    }

    if (wm_id == WM_ID_SWAY) {
        self->spec = window_manager_spec_create_sway();

        return TRUE;
    }

    if (wm_id == WM_ID_HYPRLAND) {
        self->spec = window_manager_spec_create_hyprland();

        return TRUE;
    }

    return FALSE;
}

/**
 * Handles object disposal
 *
 * @param obj The object struct
 */
static void dispose(GObject *obj) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(obj);

    if (self->event) {
        window_manager_event_destroy(self->event);
    }

    if (self->poll_data) {
        window_manager_events_poll_destroy(
            self->poll_data,
            self->spec->events_destructor
        );
    }

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->dispose(obj);
}

/**
 * Handles object finalization
 *
 * @param obj The app struct
 */
static void finalize(GObject *obj) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(obj);

    g_free(self->spec);

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->finalize(obj);
}

/**
 * Initialize the object
 *
 * @param wwt The application struct
 */
static void wwt_window_manager_init(WwtWindowManager *self) {
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_window_manager_class_init(WwtWindowManagerClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Create the window manager
 *
 * @param app The app instance
 * @param wm_id The name of window manager
 * @return The fully created window manager instance
 */
WwtWindowManager *window_manager_new(WwtApp *app, WindowManagerId wm_id) {
    WwtWindowManager *self = g_object_new(WWT_WINDOW_MANAGER_TYPE, NULL);

    self->app = app;
    self->id = wm_id;
    gboolean spec_initialized = wwt_window_manager_init_spec(self, wm_id);

    if (!spec_initialized) {
        g_object_unref(self);
        return NULL;
    }

    self->event = window_manager_event_create();
    self->poll_data = window_manager_events_poll_create(self);

    if (!self->poll_data) {
        g_object_unref(self);
        return NULL;
    }

    return self;
}
