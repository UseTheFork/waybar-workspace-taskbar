#include "cache.h"

struct _WwtCache {
    GObject parent;

    GHashTable *icons;
};

G_DEFINE_TYPE(WwtCache, wwt_cache, G_TYPE_OBJECT);

static WwtCache *instance = NULL;

/**
 * Sets the icon to the cache
 *
 * @param self
 * @param key The app_id
 * @param icon The icon
 * @return TRUE if set else FALSE
 */
gboolean wwt_cache_set_icon(WwtCache *self, const gchar *key, GIcon *icon) {
    return g_hash_table_insert(self->icons, g_strdup(key), g_object_ref(icon));
}

/**
 * Gets the cached application icon
 *
 * @param self
 * @param key The app_id
 * @return the cached icon
 */
GIcon *wwt_cache_get_icon(WwtCache *self, const gchar *key) {
    return g_hash_table_lookup(self->icons, key);
}

/**
 * Handles object disposal
 *
 * @param obj The object struct
 */
static void dispose(GObject *obj) {
    WwtCache *self = WWT_CACHE(obj);

    g_clear_pointer(&self->icons, g_hash_table_destroy);

    if(instance) {
        instance = NULL;
    }

    G_OBJECT_CLASS(wwt_cache_parent_class)->dispose(obj);
}

/**
 * Handles object finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_cache_parent_class)->finalize(obj);
}

/**
 * Initialize the object
 *
 * @param self
 */
static void wwt_cache_init(WwtCache *self) {
    self->icons =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_cache_class_init(WwtCacheClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Gets the already created instance
 *
 * @return The cache instance or NULL (Caller does not hold a reference)
 */
WwtCache *wwt_cache_instance() {
    return instance;
}

/**
 * Gets or creates the cache instance
 *
 * @return The fully created cache instance (Caller holds a reference)
 */
WwtCache *wwt_cache_default() {
    if(instance) {
        g_object_ref(instance);
        return instance;
    }

    WwtCache *self = g_object_new(WWT_CACHE_TYPE, NULL);
    instance = self;

    return self;
}
