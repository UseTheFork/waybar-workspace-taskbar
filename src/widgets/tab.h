#pragma once

#include "core/app.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WWT_TAB_TYPE (wwt_tab_get_type())
G_DECLARE_FINAL_TYPE(WwtTab, wwt_tab, WWT, TAB, GtkButton)

WwtTab *wwt_tab_new(
    WwtApp *app,
    const gchar *win_id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y
);

void wwt_tab_update(
    WwtTab *tab,
    const gchar *win_id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y
);

G_END_DECLS
