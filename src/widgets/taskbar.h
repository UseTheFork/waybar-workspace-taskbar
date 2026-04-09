#pragma once

#include "core/app.h"
#include "core/window_manager_data.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_TASKBAR_TYPE (wwt_taskbar_get_type())

G_DECLARE_FINAL_TYPE(WwtTaskbar, wwt_taskbar, WWT, TASKBAR, GtkBox)

WwtTaskbar *wwt_taskbar_new(WwtApp *app);
void wwt_taskbar_populate_tabs(WindowManagerData *wm_data, gpointer user_data);

G_END_DECLS;
