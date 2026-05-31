#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _WwtAppIcons WwtAppIcons;

#define WWT_APP_ICONS_TYPE (wwt_app_icons_get_type())

G_DECLARE_FINAL_TYPE(WwtAppIcons, wwt_app_icons, WWT, APP_ICONS, GObject);

WwtAppIcons *wwt_app_icons_new();
GdkPixbuf *wwt_app_icons_get_icon(
    WwtAppIcons *self,
    const char *app_id,
    int size
);

G_END_DECLS
