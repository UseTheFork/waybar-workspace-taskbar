#include "window_manager.h"
#include "data.h"
#include "events.h"
#include "subscriptions.h"
#include "window_managers/hyprland.h"
#include "window_managers/niri.h"
#include "window_managers/sway.h"

struct _WwtWindowManager {
    GObject parent_instance;

    WindowManagerId id;
    WwtWindowManagerData *data;
    WwtWindowManagerEvents *events;
    WwtWindowManagerSubscriptions *subscriptions;
    WindowManagerEventsConstructor events_constructor;
    WindowManagerEventsDestructor events_destructor;
    WindowManagerEventsReader events_reader;
    WindowManagerEventsValidator events_validator;
    WindowManagerDataFetcher data_fetcher;
};

G_DEFINE_TYPE(WwtWindowManager, wwt_window_manager, G_TYPE_OBJECT);

/**
 * Gets the window manager id
 *
 * @param self
 * @return The window manager id
 */
WindowManagerId wwt_window_manager_get_id(WwtWindowManager *self) {
    return self->id;
}

/**
 * Gets the window manager subscriptions
 *
 * @param self
 * @return The window manager subscriptions
 */
WwtWindowManagerSubscriptions *wwt_window_manager_get_subsciptions(
    WwtWindowManager *self
) {
    return self->subscriptions;
}

/**
 * Gets the window manager data
 *
 * @param self
 * @return The window manager data
 */
WwtWindowManagerData *wwt_window_manager_get_data(WwtWindowManager *self) {
    return self->data;
}

/**
 * Callback to call when a window manager event fires
 *
 * @param user_data The user defined data
 */
static void events_callback(gpointer user_data) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(user_data);

    wwt_window_manager_data_clear(self->data);
    self->data_fetcher(self->data);
    wwt_window_manager_subscriptions_notify(self->subscriptions, self->data);
}

/**
 * Initializes the window manager spec
 *
 * @param self
 * @return TRUE if success else FALSE
 */
static gboolean init_spec(WwtWindowManager *self) {
    WindowManagerSpecFactory factory;

    if(self->id == WM_ID_HYPRLAND) {
        factory = window_manager_spec_factory_hyprland();
    } else if(self->id == WM_ID_NIRI) {
        factory = window_manager_spec_factory_niri();
    } else if(self->id == WM_ID_SWAY) {
        factory = window_manager_spec_factory_sway();
    } else {
        return FALSE;
    }

    self->events_constructor = factory.events_constructor;
    self->events_destructor = factory.events_destructor;
    self->events_reader = factory.events_reader;
    self->events_validator = factory.events_validator;
    self->data_fetcher = factory.data_fetcher;

    return TRUE;
}

/**
 * Compose the window manager class
 *
 * @param self
 * @return TRUE if success else FALSE
 */
static gboolean compose(WwtWindowManager *self) {
    if(!init_spec(self)) {
        return FALSE;
    }

    self->subscriptions = wwt_window_manager_subscriptions_new();
    if(!self->subscriptions) {
        return FALSE;
    }

    self->data = wwt_window_manager_data_new();
    if(!self->data) {
        return FALSE;
    }

    self->events = wwt_window_manager_events_new(
        self->events_constructor,
        self->events_destructor,
        self->events_reader,
        self->events_validator,
        events_callback,
        self
    );
    if(!self->events) {
        return FALSE;
    }

    return TRUE;
}

/**
 * Class dispose method
 *
 * @param obj The instance object
 */
static void dispose(GObject *obj) {
    WwtWindowManager *self = WWT_WINDOW_MANAGER(obj);

    g_clear_object(&self->data);
    g_clear_object(&self->events);
    g_clear_object(&self->subscriptions);

    G_OBJECT_CLASS(wwt_window_manager_parent_class)->dispose(obj);
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_window_manager_init(WwtWindowManager *self) {
}

/**
 * Initialize the class
 *
 * @param klass The instance class
 */
static void wwt_window_manager_class_init(WwtWindowManagerClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
}

/**
 * Creates a new instance of the class
 *
 * @param id The window manager id
 * @return self
 */
WwtWindowManager *wwt_window_manager_new(WindowManagerId id) {
    WwtWindowManager *self = g_object_new(WWT_WINDOW_MANAGER_TYPE, NULL);
    self->id = id;

    if(!compose(self)) {
        g_object_unref(self);
        return NULL;
    }

    // Populate data on successfull init
    self->data_fetcher(self->data);

    return self;
}
