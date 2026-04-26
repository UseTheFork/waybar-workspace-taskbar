#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _AppIcons AppIcons;

AppIcons *app_icons_create();
void app_icons_destroy(AppIcons *self);
GdkPixbuf *app_icons_get_icon(AppIcons *self, const char *app_id, int size);

G_END_DECLS;
