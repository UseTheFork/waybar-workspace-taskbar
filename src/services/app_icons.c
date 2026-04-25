#include "app_icons.h"
#include <gio/gdesktopappinfo.h>

struct _AppIcons {
    GHashTable *cache;
};

/**
 * Sets the icon to the cache
 *
 * @param self
 * @param key The app_id
 * @param icon The icon
 * @return TRUE if set else FALSE
 */
static gboolean cache_set_icon(
    AppIcons *self,
    const gchar *key,
    GdkPixbuf *icon
) {
    return g_hash_table_insert(self->cache, g_strdup(key), g_object_ref(icon));
}

/**
 * Gets the cached application icon
 *
 * @param self
 * @param key The app_id
 * @return the cached icon
 */
static GdkPixbuf *cache_get_icon(AppIcons *self, const gchar *key) {
    return g_hash_table_lookup(self->cache, key);
}

/**
 * Finds the desktop info based on the app_id
 *
 * @param app_id The app_id from the window manager
 * @return the GDesktopAppInfo
 */
static GDesktopAppInfo *get_app_info(const gchar *app_id) {
    GDesktopAppInfo *info = g_desktop_app_info_new(app_id);
    if(info) {
        return info;
    }

    if(!info) {
        gchar *desktop_id = g_strdup_printf("%s.desktop", app_id);
        info = g_desktop_app_info_new(desktop_id);
        g_free(desktop_id);
    }

    gchar ***results = g_desktop_app_info_search(app_id);
    if(results && results[0] && results[0][0]) {
        info = g_desktop_app_info_new(results[0][0]);
    }

    for(gint i = 0; results && results[i]; i++) {
        g_strfreev(results[i]);
    }
    g_free(results);

    return info;
}

/**
 * Creates a pixbuf from the GIcon
 *
 * @param icon The GIcon
 * @return The created pixbuf
 */
static GdkPixbuf *create_icon_pixbuf(GIcon *icon) {
    GdkPixbuf *pixbuf = NULL;
    GtkIconTheme *theme = gtk_icon_theme_get_default();

    if(icon) {
        GtkIconInfo *info = gtk_icon_theme_lookup_by_gicon(
            theme,
            icon,
            16,
            GTK_ICON_LOOKUP_FORCE_SIZE
        );

        if(info) {
            pixbuf = gtk_icon_info_load_icon(info, NULL);
            g_object_unref(info);
        }
    }

    if(!pixbuf) {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 16, 16);
        gdk_pixbuf_fill(pixbuf, 0xEBEBEBFF);
    }

    return pixbuf;
}

/**
 * Gets the button icon either from cache or creates new
 *
 * @param self
 * @return The icon
 */
GdkPixbuf *app_icons_get_icon(AppIcons *self, const char *app_id) {
    GdkPixbuf *cached = cache_get_icon(self, app_id);

    if(cached) {
        return cached;
    }

    GDesktopAppInfo *info = get_app_info(app_id);
    if(!info) {
        return create_icon_pixbuf(NULL);
    }

    GIcon *icon = g_app_info_get_icon(G_APP_INFO(info));
    GdkPixbuf *pixbuf = create_icon_pixbuf(icon);
    cache_set_icon(self, app_id, pixbuf);

    g_object_unref(info);
    g_object_unref(pixbuf);

    return pixbuf;
}

/**
 * Destroys the app icons service
 *
 * @param self
 */
void app_icons_destroy(AppIcons *self) {
    if(self->cache) {
        g_hash_table_destroy(self->cache);
        self->cache = NULL;
    }
}

/**
 * Creates the app icons service
 *
 * @return The fully created AppIcons instance (transfer: full)
 */
AppIcons *app_icons_create() {
    AppIcons *self = g_malloc0(sizeof(AppIcons));
    self->cache =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

    return self;
}
