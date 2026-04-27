#pragma once

#include "core/app.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_TASKBAR_TYPE (wwt_taskbar_get_type())

G_DECLARE_FINAL_TYPE(WwtTaskbar, wwt_taskbar, WWT, TASKBAR, GtkBox)

typedef enum TaskbarFocusDirection {
    TASKBAR_FOCUS_PREV,
    TASKBAR_FOCUS_NEXT
} TaskbarFocusDirection;

WwtTaskbar *wwt_taskbar_new(WwtApp *app);
void wwt_taskbar_update_tabs(WwtTaskbar *self);
void wwt_taskbar_shift_focus(WwtTaskbar *self, TaskbarFocusDirection direction);

G_END_DECLS;
