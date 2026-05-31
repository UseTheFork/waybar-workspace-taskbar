#include "data.h"

#define WINDOWS_PTR_ARRAY_RESERVED_SIZE 20

struct _WwtWindowManagerData {
    GObject parent_instance;

    int focused_workspace_id;
    GHashTable *workspaces;
};

typedef struct {
    int id;
    int focused;
    gchar *output;
    GPtrArray *windows;
} WindowManagerDataWorkspace;

G_DEFINE_TYPE(WwtWindowManagerData, wwt_window_manager_data, G_TYPE_OBJECT);

/**
 * Predicate function for output name lookup
 *
 * @param key The hash table key
 * @param value The hash table value
 * @param user_data The user data passed ( output ) in this case
 * @return TRUE if equal else FALSE
 */
static gboolean workspace_output_lookup_predicate(
    gpointer key,
    gpointer value,
    gpointer user_data
) {
    const char *output = user_data;
    WindowManagerDataWorkspace *ws = value;

    return strcmp(ws->output, output) == 0;
}

/**
 * Gets a workspace on an output if specified otherwise gets the focused
 * workspace
 *
 * @param self
 * @param output The output to find workspace on can be NULL
 */
static WindowManagerDataWorkspace *get_workspace_on_output(
    WwtWindowManagerData *self,
    const char *output
) {
    if(!output) {
        return g_hash_table_lookup(
            self->workspaces,
            GINT_TO_POINTER(self->focused_workspace_id)
        );
    }

    return g_hash_table_find(
        self->workspaces,
        workspace_output_lookup_predicate,
        (gpointer)output
    );
}

/**
 * Gets the focused index on a workspace
 *
 * @param self
 * @param output The output if NULL defaults to focused
 */
int wwt_window_manager_data_get_focused_idx(
    WwtWindowManagerData *self,
    const char *output
) {
    WindowManagerDataWorkspace *ws = get_workspace_on_output(self, output);

    if(!ws) {
        return -1;
    }

    for(int i = 0; i < (int)ws->windows->len; i++) {
        WindowManagerDataWindow *win = g_ptr_array_index(ws->windows, i);

        if(win->focused) {
            return i;
        }
    }

    return -1;
}

/**
 * Gets the focused window id
 *
 * @param self
 * @param output The output name or if NULL get from focused workspace
 * @param offset The offset from the focused window 0 = current, -1 = prev, 1 =
 * next
 * @return The window id
 */
WindowManagerDataWindow *wwt_window_manager_data_get_window_at_idx(
    WwtWindowManagerData *self,
    const char *output,
    int idx
) {
    WindowManagerDataWorkspace *ws = get_workspace_on_output(self, output);

    if(!ws) {
        return NULL;
    }

    idx = CLAMP(idx, 0, (int)ws->windows->len - 1);
    WindowManagerDataWindow *win = g_ptr_array_index(ws->windows, idx);

    return win;
}

/**
 * Gets windows from an output if specified otherwise from the focused workspace
 *
 * @param self
 * @param output The output name
 * @return The windows array or NULL if not found (Window Manager Data owns the
 * array)
 */
GPtrArray *wwt_window_manager_data_get_windows(
    WwtWindowManagerData *self,
    const char *output
) {
    WindowManagerDataWorkspace *ws = get_workspace_on_output(self, output);

    if(!ws) {
        return NULL;
    }

    return ws->windows;
}

/**
 * HFunc callback for the foreach in sort windows
 *
 * @param key
 * @param value
 * @param user_data
 */
static void sort_foreach_fn(gpointer key, gpointer value, gpointer user_data) {
    WindowManagerDataWorkspace *ws = value;
    GCompareFunc compare_fn = user_data;

    g_ptr_array_sort(ws->windows, compare_fn);
}

/**
 * Sorts all windows in every workspace
 *
 * @param compare_fn The compare function to check against
 */
void wwt_window_manager_data_sort_windows(
    WwtWindowManagerData *self,
    GCompareFunc compare_fn
) {
    g_hash_table_foreach(self->workspaces, sort_foreach_fn, compare_fn);
}

/**
 * Destroy a window manager window
 *
 * @param data Data passed from GDestroyNotify in this case the window
 */
static void window_destroy(gpointer data) {
    WindowManagerDataWindow *win = data;

    g_free(win->id);
    g_free(win->title);
    g_free(win->app_id);
    g_free(win);
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
WindowManagerDataWindow *window_create(
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int workspace_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y,
    int sortable
) {
    WindowManagerDataWindow *win = g_malloc(sizeof(WindowManagerDataWindow));
    win->id = g_strdup(id);
    win->title = g_strdup(title);
    win->app_id = g_strdup(app_id);
    win->workspace_id = workspace_id;
    win->focused = focused;
    win->floating = floating;
    win->urgent = urgent;
    win->x = x;
    win->y = y;
    win->sortable = sortable;

    return win;
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
gboolean wwt_window_manager_data_window_add(
    WwtWindowManagerData *self,
    const gchar *id,
    const gchar *title,
    const gchar *app_id,
    int workspace_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y,
    int sortable
) {
    WindowManagerDataWorkspace *ws =
        g_hash_table_lookup(self->workspaces, GINT_TO_POINTER(workspace_id));

    if(!ws) {
        return FALSE;
    }

    WindowManagerDataWindow *win = window_create(
        id,
        title,
        app_id,
        workspace_id,
        focused,
        floating,
        urgent,
        x,
        y,
        sortable
    );

    g_ptr_array_add(ws->windows, win);

    return TRUE;
}

/**
 * Sets the focused workspace
 *
 * @param self
 * @param id The id for the workspace
 */
void wwt_window_manager_data_set_focused_workspace(
    WwtWindowManagerData *self,
    int id
) {
    WindowManagerDataWorkspace *ws =
        g_hash_table_lookup(self->workspaces, GINT_TO_POINTER(id));

    if(!ws) {
        return;
    }

    self->focused_workspace_id = id;
    ws->focused = TRUE;
}

/**
 * Destroys a workspace
 *
 * @param data Data passed from GDestroyNotify in this case the workspace
 */
static void workspace_destroy(gpointer data) {
    WindowManagerDataWorkspace *ws = data;

    g_free(ws->output);
    g_ptr_array_free(ws->windows, TRUE);
    g_free(ws);
}

/**
 * Creates a workspace
 *
 * @param id The workspace id
 * @param focused Is the workspace focused
 * @param output The output the workspace is on
 * @param layout The workspace layout
 * @return The created window
 */
static WindowManagerDataWorkspace *workspace_create(
    int id,
    int focused,
    const char *output
) {
    WindowManagerDataWorkspace *ws =
        g_malloc(sizeof(WindowManagerDataWorkspace));
    ws->id = id;
    ws->focused = focused;
    ws->output = g_strdup(output);

    ws->windows =
        g_ptr_array_new_full(WINDOWS_PTR_ARRAY_RESERVED_SIZE, window_destroy);

    return ws;
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
gboolean wwt_window_manager_data_workspace_add(
    WwtWindowManagerData *self,
    int id,
    int focused,
    const char *output
) {
    WindowManagerDataWorkspace *ws = workspace_create(id, focused, output);

    if(focused) {
        self->focused_workspace_id = id;
    }

    return g_hash_table_insert(self->workspaces, GINT_TO_POINTER(id), ws);
}

/**
 * GDestroyNotify function for the hash table
 *
 * @param data The table item
 */
static void workspaces_table_items_destroy_notify(gpointer data) {
    WindowManagerDataWorkspace *ws = data;
    workspace_destroy(ws);
}

/**
 * Clears the window manager data to the initial state
 *
 * @param self
 */
void wwt_window_manager_data_clear(WwtWindowManagerData *self) {
    self->focused_workspace_id = -1;
    g_hash_table_remove_all(self->workspaces);
}

/**
 * Dispose the instance
 *
 * @param obj The instance object
 */
static void dispose(GObject *obj) {
    WwtWindowManagerData *self = WWT_WINDOW_MANAGER_DATA(obj);

    if(self->workspaces) {
        g_hash_table_destroy(self->workspaces);
        self->workspaces = NULL;
    }

    G_OBJECT_CLASS(wwt_window_manager_data_parent_class)->dispose(obj);
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_window_manager_data_init(WwtWindowManagerData *self) {
    self->focused_workspace_id = -1;
    self->workspaces = g_hash_table_new_full(
        g_direct_hash,
        g_direct_equal,
        NULL,
        workspaces_table_items_destroy_notify
    );
}

/**
 * Initialize the class
 *
 * @param klass the object class
 */
static void wwt_window_manager_data_class_init(
    WwtWindowManagerDataClass *klass
) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
}

/**
 * Creates a new instance
 *
 * @param data_fetcher The function to be used to fetch data from the window
 * manager
 * @return self
 */
WwtWindowManagerData *wwt_window_manager_data_new() {
    WwtWindowManagerData *self =
        g_object_new(WWT_WINDOW_MANAGER_DATA_TYPE, NULL);

    return self;
}
