#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_CACHE_TYPE (wwt_cache_get_type())

G_DECLARE_FINAL_TYPE(WwtCache, wwt_cache, WWT, CACHE, GObject)

WwtCache *wwt_cache_default();
WwtCache *wwt_cache_instance();
gboolean wwt_cache_set_icon(WwtCache *self, const gchar *key, GIcon *value);
GIcon *wwt_cache_get_icon(WwtCache *self, const gchar *key);

G_END_DECLS;
