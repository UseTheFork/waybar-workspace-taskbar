#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _WindowManagerData WindowManagerData;
typedef struct _WindowManagerWorkspace WindowManagerWorkspace;

typedef struct {
    gchar *id;
    gchar *title;
    gchar *app_id;
    int ws_id;
    int focused;
    int floating;
    int urgent;
    int x;
    int y;
    int sortable;
} WindowManagerWindow;

WindowManagerData *window_manager_data_create();
void window_manager_data_destroy(WindowManagerData *self);

GPtrArray *window_manager_data_get_windows_on_focused(WindowManagerData *self);
GPtrArray *window_manager_data_get_windows_on_output(
    WindowManagerData *self,
    const char *output
);

gboolean window_manager_data_window_create(
    WindowManagerData *self,
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

gboolean window_manager_data_workspace_create(
    WindowManagerData *self,
    int id,
    int focused,
    const gchar *output
);

void window_manager_data_sort_windows(
    WindowManagerData *self,
    GCompareFunc compare_fn
);

void window_manager_data_set_focused_workspace(WindowManagerData *self, int id);

G_END_DECLS
