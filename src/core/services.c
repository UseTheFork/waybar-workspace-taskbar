#include "services.h"
#include "config.h"
#include "services/app_icons.h"
#include "services/window_manager_events.h"
#include "services/window_manager_spec.h"

typedef enum InitStatus {
    INIT_STATUS_PENDING,
    INIT_STATUS_FAILED,
    INIT_STATUS_SUCCESS
} InitStatus;

struct _WwtServices {
    GObject parent_instance;

    WindowManagerSpec *window_manager_spec;
    WindowManagerEvents *window_manager_events;
    AppIcons *app_icons;
};

G_DEFINE_TYPE(WwtServices, wwt_services, G_TYPE_OBJECT);

static WwtServices *instance = NULL;
static InitStatus init_status = INIT_STATUS_PENDING;

/**
 * Gets the app icons
 *
 * @param self
 * @return The app icons
 */
AppIcons *wwt_services_get_app_icons(WwtServices *self) {
    return self->app_icons;
}

/**
 * Gets the window manager events
 *
 * @param self
 * @return The window events
 */
WindowManagerEvents *wwt_services_get_window_manager_events(WwtServices *self) {
    return self->window_manager_events;
}

/**
 * Gets the window manager events
 *
 * @param self
 * @return The window events
 */
WindowManagerSpec *wwt_services_get_window_manager_spec(WwtServices *self) {
    return self->window_manager_spec;
}

/**
 * Handles object disposal
 *
 * @param obj The object struct
 */
static void dispose(GObject *obj) {
    WwtServices *self = WWT_SERVICES(obj);

    if(self->window_manager_events) {
        window_manager_events_destroy(self->window_manager_events);
        self->window_manager_events = NULL;
    }

    if(self->window_manager_spec) {
        window_manager_spec_destroy(self->window_manager_spec);
        self->window_manager_spec = NULL;
    }

    if(self->app_icons) {
        app_icons_destroy(self->app_icons);
        self->app_icons = NULL;
    }

    if(instance) {
        instance = NULL;
        init_status = INIT_STATUS_PENDING;
    }

    G_OBJECT_CLASS(wwt_services_parent_class)->dispose(obj);
}

/**
 * Handles object finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_services_parent_class)->finalize(obj);
}

/**
 * Initialize the object
 *
 * @param self
 */
static void wwt_services_init(WwtServices *self) {
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_services_class_init(WwtServicesClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Gets or creates the services instance
 *
 * @param config The WwtConfig instance
 * @return The fully created services instance (Caller holds a reference)
 */
WwtServices *wwt_services_default(WwtConfig *config) {
    if(init_status == INIT_STATUS_FAILED) {
        return NULL;
    }

    if(instance && init_status == INIT_STATUS_SUCCESS) {
        g_object_ref(instance);
        return instance;
    }

    WwtServices *self = g_object_new(WWT_SERVICES_TYPE, NULL);

    WindowManagerId wm_id = wwt_config_get_wm_id(config);
    self->window_manager_spec = window_manager_spec_create(wm_id);
    if(!self->window_manager_spec) {
        init_status = INIT_STATUS_FAILED;
        g_object_unref(self);
        return NULL;
    }

    self->window_manager_events =
        window_manager_events_create(self->window_manager_spec);
    if(!self->window_manager_events) {
        init_status = INIT_STATUS_FAILED;
        g_object_unref(self);
        return NULL;
    }

    self->app_icons = app_icons_create();
    if(!self->app_icons) {
        init_status = INIT_STATUS_FAILED;
        g_object_unref(self);
        return NULL;
    }

    instance = self;
    init_status = INIT_STATUS_SUCCESS;

    return self;
}
