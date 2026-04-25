#include "window_manager.h"
#include "window_manager_events.h"
#include "window_manager_spec.h"

#define WM_EVENTS_POLLING_TIMEOUT 20
#define WM_EVENTS_DEBOUNCE_TIMEOUT 20
#define WM_EVENTS_MSG_SIZE 4096
#define WM_EVENTS_MAX_CALlBACKS 8

enum WwtWindowManagerInitStatus {
    WM_INIT_STATUS_PENDING,
    WM_INIT_STATUS_FAILED,
    WM_INIT_STATUS_SUCCESS
};

struct _WwtWindowManager {
    GObject parent;

    WindowManagerId id;
    WindowManagerSpec *spec;
    WindowManagerEvents *events;
};

G_DEFINE_TYPE(WwtWindowManager, wwt_window_manager, G_TYPE_OBJECT);

static WwtWindowManager *instance = NULL;
static int init_status = WM_INIT_STATUS_PENDING;

/**
 * Gets the window manager spec
 *
 * @param self
 * @return The window manager spec
 */
WindowManagerSpec *wwt_window_manager_get_spec(WwtWindowManager *self) {
    return self->spec;
}

/**
 * Gets the window manager events
 *
 * @param self
 * @return The window manager events
 */
WindowManagerEvents *wwt_window_manager_get_events(WwtWindowManager *self) {
    return self->events;
}

/**
 * Handles object disposal
 *
 * @param obj The object struct
 */
static void dispose(GObject *obj) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(obj);

    if(self->events) {
        window_manager_events_destroy(self->events);
        self->events = NULL;
    }

    if(self->spec) {
        window_manager_spec_destroy(self->spec);
        self->spec = NULL;
    }

    if(instance) {
        instance = NULL;
        init_status = WM_INIT_STATUS_PENDING;
    }

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->dispose(obj);
}

/**
 * Handles object finalization
 *
 * @param obj The struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_window_manager_parent_class)->finalize(obj);
}

/**
 * Initialize the object
 *
 * @param self
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
 * Gets the already created instance
 *
 * @return The window manager instance or NULL (Caller does not hold a
 * reference)
 */
WwtWindowManager *wwt_window_manager_instance() {
    return instance;
}

/**
 * Gets or creates the window manager instance
 *
 * @param wm_id The name of window manager
 * @return The fully created window manager instance (Caller holds a reference
 * and must unref when finished)
 */
WwtWindowManager *wwt_window_manager_default(WindowManagerId wm_id) {
    if(init_status == WM_INIT_STATUS_FAILED) {
        return NULL;
    }

    if(instance && init_status == WM_INIT_STATUS_SUCCESS) {
        g_object_ref(instance);
        return instance;
    }

    WwtWindowManager *self = g_object_new(WWT_WINDOW_MANAGER_TYPE, NULL);

    self->id = wm_id;
    self->spec = window_manager_spec_create(wm_id);

    if(!self->spec) {
        init_status = WM_INIT_STATUS_FAILED;
        g_object_unref(self);
        return NULL;
    }

    self->events = window_manager_events_create(self->spec);

    if(!self->events) {
        init_status = WM_INIT_STATUS_FAILED;
        g_object_unref(self);
        return NULL;
    }

    instance = self;
    init_status = WM_INIT_STATUS_SUCCESS;
    return self;
}
