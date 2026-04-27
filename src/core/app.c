#include "app.h"
#include "config.h"
#include "services.h"
#include "services/window_manager_spec.h"
#include "wbcffi.h"
#include "widgets/taskbar.h"

struct _WwtApp {
    GObject parent;

    wbcffi_module *waybar_module;
    GtkContainer *root_widget;
    WwtTaskbar *taskbar;
    WwtConfig *config;
    WwtServices *services;
};

G_DEFINE_TYPE(WwtApp, wwt_app, G_TYPE_OBJECT);

/**
 * Gets the services
 *
 * @param self
 * @return The services
 */
WwtServices *wwt_app_get_services(WwtApp *self) {
    return self->services;
}

/**
 * Gets the taskbar widget
 *
 * @param self
 * @return The taskbar
 */
WwtTaskbar *wwt_app_get_taskbar(WwtApp *self) {
    return self->taskbar;
}

/**
 * Gets the config instance
 *
 * @param self
 * @return The app config
 */
WwtConfig *wwt_app_get_config(WwtApp *self) {
    return self->config;
}

/**
 * Handles obj disposal
 *
 * @param obj The obj struct
 */
static void dispose(GObject *obj) {
    WwtApp *self = WWT_APP(obj);

    if(self->taskbar) {
        gtk_container_remove(
            GTK_CONTAINER(self->root_widget),
            GTK_WIDGET(self->taskbar)
        );

        self->taskbar = NULL;
    }

    g_clear_object(&self->config);
    g_clear_object(&self->services);

    G_OBJECT_CLASS(wwt_app_parent_class)->dispose(obj);
}

/**
 * Handles obj finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_app_parent_class)->finalize(obj);
}

/**
 * Initialize the app
 *
 * @param self
 */
static void wwt_app_init(WwtApp *self) {
}

/**
 * Initialized the class instance and methods
 *
 * @param class The class instance
 */
static void wwt_app_class_init(WwtAppClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Creates a new instance of the app
 *
 * @param init_info Initiaization info from waybar
 * @param config_entries The configuration entries from the module
 * @param config_entries_len The len of config entries
 * @return The fully created app instance
 */
WwtApp *wwt_app_new(
    const wbcffi_init_info *init_info,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    WwtApp *self = g_object_new(WWT_APP_TYPE, NULL);
    self->waybar_module = init_info->obj;
    self->config = wwt_config_new(config_entries, config_entries_len);

    WindowManagerId wm_id = wwt_config_get_wm_id(self->config);
    if(wm_id == WM_ID_UNSUPPORTED) {
        g_object_unref(self);
        return NULL;
    }

    self->services = wwt_services_default(self->config);
    if(!self->services) {
        g_object_unref(self);
        g_critical("Waybar Workspace Taskbar: error initializing services");
        return NULL;
    }

    self->taskbar = wwt_taskbar_new(self);
    if(!self->taskbar) {
        g_object_unref(self);
        g_critical("Waybar Workspace Taskbar: error initializing taskbar");
        return NULL;
    }

    self->root_widget = init_info->get_root_widget(init_info->obj);
    gtk_container_add(
        GTK_CONTAINER(self->root_widget),
        GTK_WIDGET(self->taskbar)
    );

    return self;
}
