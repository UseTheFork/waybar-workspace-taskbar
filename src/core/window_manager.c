#include "window_manager.h"
#include "window_manager_events.h"
#include "window_managers/niri.h"
#include <fcntl.h>
#include <sys/poll.h>

#define WM_EVENTS_MAX_CALlBACKS 8

struct _WwtWindowManager {
    GObject parent;

    WwtApp *app;
    WindowManagerId id;
    WindowManagerSpec *spec;
    WindowManagerEvents *events;
    int events_subscription_id;
};

G_DEFINE_TYPE(WwtWindowManager, wwt_window_manager, G_TYPE_OBJECT);

/**
 * Gets the generate tabs function from the window manager spec
 *
 * @param self The window manager struct
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
 * Initiaizes the window manager spec
 *
 * @param self
 * @param wm_id The window manager id
 */
static gboolean wwt_window_manager_init_spec(
    WwtWindowManager *self,
    WindowManagerId wm_id
) {
    if (wm_id == WM_ID_NIRI) {
        self->spec = window_manager_spec_create_niri();

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

    printf("calling dispose on window manager\n");

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->dispose(obj);
}

/**
 * Handles object finalization
 *
 * @param obj The app struct
 */
static void finalize(GObject *obj) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(obj);

    window_manager_events_unsubscribe(
        self->events,
        self->events_subscription_id
    );

    g_free(self->spec);
    g_free(self->events);

    printf("calling finalize on window manager\n");

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->finalize(obj);
}

/**
 * Initialize the object
 *
 * @param wwt The application struct
 */
static void wwt_window_manager_init(WwtWindowManager *self) {
    printf("calling init on window manager\n");
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
 * @param wm_id The name of window manager
 */
WwtWindowManager *window_manager_new(WwtApp *app, WindowManagerId wm_id) {
    WwtWindowManager *self = g_object_new(WWT_WINDOW_MANAGER_TYPE, NULL);

    self->app = app;
    self->id = wm_id;
    gboolean spec_initialized = wwt_window_manager_init_spec(self, wm_id);

    if (!spec_initialized) {
        g_clear_object(&self);
        return NULL;
    }

    self->events = window_manager_events_create(
        self->spec->socket_init,
        self->spec->events_buf_size
    );

    if (!self->events) {
        g_clear_object(&self);
        return NULL;
    }

    self->events_subscription_id = window_manager_events_subscribe(
        self->events,
        self->spec->events_callback,
        app
    );

    return self;
}
