#include "window_manager_data.h"

#define WINDOWS_PTR_ARRAY_RESERVED_SIZE 20

struct _WindowManagerData {
    GObject parent_instance;

    int focused_workspace_id;
    GHashTable *workspaces;
};

struct _WindowManagerWorkspace {
    int id;
    int focused;
    gchar *output;
    GPtrArray *windows;
};

/**
 * Sets the focused workspace
 *
 * @param self
 * @param id The id for the workspace
 */
void window_manager_data_set_focused_workspace(
    WindowManagerData *self,
    int id
) {
    WindowManagerWorkspace *ws =
        g_hash_table_lookup(self->workspaces, GINT_TO_POINTER(id));

    if(!ws) {
        return;
    }

    self->focused_workspace_id = id;
    ws->focused = TRUE;
}

/**
 * HFunc callback for the foreach in sort windows
 *
 * @param key
 * @param value
 * @param user_data
 */
static void sort_foreach_fn(gpointer key, gpointer value, gpointer user_data) {
    WindowManagerWorkspace *ws = value;
    GCompareFunc compare_fn = user_data;

    g_ptr_array_sort(ws->windows, compare_fn);
}

/**
 * Sorts all windows in every workspace
 *
 * @param compare_fn The compare function to check against
 */
void window_manager_data_sort_windows(
    WindowManagerData *self,
    GCompareFunc compare_fn
) {
    g_hash_table_foreach(self->workspaces, sort_foreach_fn, compare_fn);
}

/**
 * Predicate function for output name lookup
 *
 * @param key The hash table key
 * @param value The hash table value
 * @param user_data The user data passed ( output ) in this case
 * @return TRUE if equal else FALSE
 */
static gboolean output_lookup_predicate(
    gpointer key,
    gpointer value,
    gpointer user_data
) {
    const char *output = user_data;
    WindowManagerWorkspace *ws = value;

    return strcmp(ws->output, output) == 0;
}

/**
 * Gets the windows from a specific output
 *
 * @param self
 * @param output The output name
 * @return The windows array or NULL if not found (Window Manager Data owns the
 * array)
 */
GPtrArray *window_manager_data_get_windows_on_output(
    WindowManagerData *self,
    const char *output
) {
    WindowManagerWorkspace *ws = g_hash_table_find(
        self->workspaces,
        output_lookup_predicate,
        (gpointer)output
    );

    if(!ws) {
        return NULL;
    }

    return ws->windows;
}

/**
 * Gets the windows from the focused workspaces
 *
 * @param self
 * @return The windows array or NULL if not found (Window Manager Data owns the
 * array)
 */
GPtrArray *window_manager_data_get_windows_on_focused(WindowManagerData *self) {
    if(self->focused_workspace_id < 0) {
        return NULL;
    }

    WindowManagerWorkspace *ws = g_hash_table_lookup(
        self->workspaces,
        GINT_TO_POINTER(self->focused_workspace_id)
    );

    if(!ws) {
        return NULL;
    }

    return ws->windows;
}

/**
 * Destroy a window manager window
 *
 * @param data Data passed from GDestroyNotify in this case the window
 */
static void window_manager_window_destroy(gpointer data) {
    WindowManagerWindow *win = data;

    g_free(win->id);
    g_free(win->title);
    g_free(win->app_id);
    g_free(win);
}

/**
 * Destroys a workspace
 *
 * @param data Data passed from GDestroyNotify in this case the workspace
 */
static void window_manager_workspace_destroy(gpointer data) {
    WindowManagerWorkspace *ws = data;

    g_free(ws->output);
    g_ptr_array_free(ws->windows, TRUE);
    g_free(ws);
}

/**
 * Creates a window and adds to a workspace
 *
 * @param self
 * @param id The tabs id (should be the same as the compositor window)
 * @param title The window title
 * @param app_id The application id or class
 * @param focused The focused status
 * @param floating The floating status
 * @param urgent The urgent status
 * @param x Window position x
 * @param y Window position y
 * @param sortable A backup sortable value in case you don't want to sort by
 * window pos
 * @return TRUE if inserted else FALSE
 */
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
) {
    WindowManagerWorkspace *ws =
        g_hash_table_lookup(self->workspaces, GINT_TO_POINTER(ws_id));

    if(!ws) {
        return FALSE;
    }

    WindowManagerWindow *win = g_malloc(sizeof(WindowManagerWindow));

    win->id = g_strdup(id);
    win->title = g_strdup(title);
    win->app_id = g_strdup(app_id);
    win->ws_id = ws_id;
    win->focused = focused;
    win->floating = floating;
    win->urgent = urgent;
    win->x = x;
    win->y = y;
    win->sortable = sortable;

    g_ptr_array_add(ws->windows, win);

    return TRUE;
}

/**
 * Creates a workspace and adds to the workspaces
 *
 * @param self
 * @param id The workspace id
 * @param focused Is the workspace focused
 * @param output The output the workspace is on
 * @param layout The workspace layout
 * @return TRUE if inserted else FALSE
 */
gboolean window_manager_data_workspace_create(
    WindowManagerData *self,
    int id,
    int focused,
    const char *output
) {
    WindowManagerWorkspace *ws = g_malloc(sizeof(WindowManagerWorkspace));
    ws->id = id;
    ws->focused = focused;
    ws->output = g_strdup(output);

    if(focused) {
        self->focused_workspace_id = id;
    }

    ws->windows = g_ptr_array_new_full(
        WINDOWS_PTR_ARRAY_RESERVED_SIZE,
        window_manager_window_destroy
    );

    return g_hash_table_insert(self->workspaces, GINT_TO_POINTER(id), ws);
}

/**
 * GDestroyNotify function for the hash table
 *
 * @param data The table item
 */
static void table_items_destroy_notify(gpointer data) {
    WindowManagerWorkspace *ws = data;
    window_manager_workspace_destroy(ws);
}

/**
 * Starts the destruction of the window manager data
 *
 * @param self
 */
void window_manager_data_destroy(WindowManagerData *self) {
    if(!self) {
        return;
    }

    if(self->workspaces) {
        g_hash_table_destroy(self->workspaces);
        self->workspaces = NULL;
    }

    g_free(self);
}

/**
 * Create the window manager workspaces instance
 *
 * @return self
 */
WindowManagerData *window_manager_data_create() {
    WindowManagerData *self = g_malloc(sizeof(WindowManagerData));

    self->focused_workspace_id = -1;
    self->workspaces = g_hash_table_new_full(
        g_direct_hash,
        g_direct_equal,
        NULL,
        table_items_destroy_notify
    );

    return self;
}
