#pragma once

#include "core/app.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WWT_TASKBAR_TYPE (wwt_taskbar_get_type())

G_DECLARE_FINAL_TYPE(WwtTaskbar, wwt_taskbar, WWT, TASKBAR, GtkBox)

WwtTaskbar *wwt_taskbar_new(WwtApp *app);
void wwt_taskbar_update_tabs(WwtTaskbar *self);
gchar *wwt_taskbar_get_focus_win_id(WwtTaskbar *self, int offset);

G_END_DECLS
