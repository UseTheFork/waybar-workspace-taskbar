#pragma once

#include "window_manager.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WWT_WINDOW_MANAGER_DATA_TYPE (wwt_window_manager_data_get_type())

G_DECLARE_FINAL_TYPE(
    WwtWindowManagerData,
    wwt_window_manager_data,
    WWT,
    WINDOW_MANAGER_DATA,
    GObject
);

typedef struct {
    gchar *id;
    gchar *title;
    gchar *app_id;
    int workspace_id;
    int focused;
    int floating;
    int urgent;
    int x;
    int y;
    int sortable;
} WindowManagerDataWindow;

WwtWindowManagerData *wwt_window_manager_data_new();

gboolean wwt_window_manager_data_workspace_add(
    WwtWindowManagerData *self,
    int id,
    int focused,
    const char *output
);

void wwt_window_manager_data_set_focused_workspace(
    WwtWindowManagerData *self,
    int id
);

gboolean wwt_window_manager_data_window_add(
    WwtWindowManagerData *self,
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int ws_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y,
    int sortable
);

void wwt_window_manager_data_sort_windows(
    WwtWindowManagerData *self,
    GCompareFunc compare_fn
);

void wwt_window_manager_data_clear(WwtWindowManagerData *self);

GPtrArray *wwt_window_manager_data_get_windows(
    WwtWindowManagerData *self,
    const char *output
);

WindowManagerDataWindow *wwt_window_manager_data_get_window_at_idx(
    WwtWindowManagerData *self,
    const char *output,
    int idx
);

int wwt_window_manager_data_get_focused_idx(
    WwtWindowManagerData *self,
    const char *output
);

G_END_DECLS
