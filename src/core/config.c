#include "config.h"

struct _WwtConfig {
    GObject parent;
};

G_DEFINE_TYPE(WwtConfig, wwt_config, G_TYPE_OBJECT);

/**
 * Handles obj disposal
 *
 * @param obj The obj struct
 */
static void dispose(GObject *obj) {
    WwtConfig *self = WWT_CONFIG(obj);

    printf("calling dipsose on config\n");

    G_OBJECT_CLASS(wwt_config_parent_class)->dispose(obj);
}

/**
 * Handles obj finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    WwtConfig *self = WWT_CONFIG(obj);

    printf("calling finalize on config\n");

    G_OBJECT_CLASS(wwt_config_parent_class)->finalize(obj);
}

/**
 * Initialize the config
 *
 * @param self
 */
static void wwt_config_init(WwtConfig *self) {
    printf("calling init on config\n");
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_config_class_init(WwtConfigClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Creates a new instance of the config
 */
WwtConfig *wwt_config_new(
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    WwtConfig *self = g_object_new(WWT_CONFIG_TYPE, NULL);

    return self;
}
