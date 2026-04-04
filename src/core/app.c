#include "app.h"
#include "config.h"
#include "wbcffi.h"
#include "widgets/tabs.h"
#include "window_manager.h"

struct _WwtApp {
    GObject parent;

    wbcffi_module *waybar_module;
    WwtConfig *config;
    WwtTabs *tabs;
    WwtWindowManager *window_manager;
};

G_DEFINE_TYPE(WwtApp, wwt_app, G_TYPE_OBJECT);

/**
 * Gets the tabs instance
 *
 * @param self
 */
WwtTabs *wwt_app_get_tabs(WwtApp *self) {
    return self->tabs;
}

/**
 * Gets the config instance
 *
 * @param self
 */
WwtConfig *wwt_app_get_config(WwtApp *self) {
    return self->config;
}

/**
 * Gets the config instance
 *
 * @param self
 */
WwtWindowManager *wwt_app_get_window_manager(WwtApp *self) {
    return self->window_manager;
}

/**
 * Handles obj disposal
 *
 * @param obj The obj struct
 */
static void dispose(GObject *obj) {
    WwtApp *self = WWT_APP(obj);

    g_clear_object(&self->config);
    g_clear_object(&self->window_manager);

    printf("calling dipsose on app\n");

    G_OBJECT_CLASS(wwt_app_parent_class)->dispose(obj);
}

/**
 * Handles obj finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    WwtApp *self = WWT_APP(obj);

    printf("calling finalize on app\n");

    G_OBJECT_CLASS(wwt_app_parent_class)->finalize(obj);
}

/**
 * Initialize the app
 *
 * @param wwt The application struct
 */
static void wwt_app_init(WwtApp *self) {
    printf("calling init on app\n");
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_app_class_init(WwtAppClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Creates a new instance of the app
 */
WwtApp *wwt_app_new(
    const wbcffi_init_info *init_info,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    WwtApp *self = g_object_new(WWT_APP_TYPE, NULL);

    self->waybar_module = init_info->obj;
    self->config = wwt_config_new(config_entries, config_entries_len);
    self->window_manager = window_manager_new(self, WM_ID_NIRI);

    // Add a container for displaying the tabs
    GtkContainer *root = init_info->get_root_widget(init_info->obj);
    self->tabs = wwt_tabs_new(self);
    gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(self->tabs));

    return self;
}
