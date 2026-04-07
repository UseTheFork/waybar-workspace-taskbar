#include "niri.h"
#include "common.h"
#include "core/config.h"
#include "core/utils.h"
#include "glib.h"
#include "widgets/taskbar.h"
#include <stdio.h>

/**
 * Connect and initialize the socket connection
 *
 * @return The file descriptor
 */
static int events_constructor() {
    const char *socket_path = getenv("NIRI_SOCKET");

    if (!socket_path) {
        return -1;
    }

    int fd = socket_connect(socket_path);

    const char *req = "\"EventStream\"\n";
    write(fd, req, strlen(req));

    return fd;
}

/**
 * Close the socket connection
 *
 * @param fd The socket file descriptor
 * @param socket_file The socket file
 */
static void events_destructor(int fd, FILE *socket_file) {
    fclose(socket_file);
}

/**
 * Read and parse events. Add any data to the window manager event and return
 * TRUE when the event is ready to be emitted
 *
 * @param socket_file The socket file to process
 * @param event The event to process into
 * @return TRUE if should emit the event else FALSE
 */
static gboolean events_reader(FILE *socket_file, WindowManagerEvent *event) {
    if (getline(&event->msg, &event->msg_size, socket_file) <= 0) {
        clearerr(socket_file);
        return FALSE;
    }

    event->msg_len = strlen(event->msg);

    return TRUE;
}

/**
 * Fires after the debounce period. This is where all the actual work is done
 *
 * @param user_data any data that was passed in when subscribe was called. app
 * in this instance
 * @return FALSE if the source should be removed. G_SOURCE_CONTINUE and
 * G_SOURCE_REMOVE are more memorable names for the return value.
 */
static gboolean events_debounce_callback(gpointer user_data) {
    DebounceCallbackData *callback_data = user_data;
    WwtApp *app = callback_data->app;
    WindowManagerEvent *event = callback_data->event;
    WwtTaskbar *taskbar = wwt_app_get_taskbar(app);

    wwt_taskbar_generate_tabs(taskbar);

    event->debounce_timeout_id = 0;

    return G_SOURCE_REMOVE;
}

/**
 * Callback that fires when the full event message is ready
 *
 * @param event The event instance
 * @param user_data any data that was passed in when subscribe was called. app
 * in this instance
 */
static void events_callback(WindowManagerEvent *event, WwtApp *app) {
    JsonParser *parser = create_json_parser(event->msg);
    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);

    if (json_object_has_member(root_obj, "WindowOpenedOrChanged") ||
        json_object_has_member(root_obj, "WorkspacesChanged") ||
        json_object_has_member(root_obj, "WindowFocusChanged") ||
        json_object_has_member(root_obj, "WorkspaceActivated") ||
        json_object_has_member(root_obj, "WorkspaceActiveWindowChanged") ||
        json_object_has_member(root_obj, "WorkspaceActiveWindowChanged") ||
        json_object_has_member(root_obj, "OverviewOpenedOrClosed")) {

        if (event->debounce_timeout_id != 0) {
            g_source_remove(event->debounce_timeout_id);
            event->debounce_timeout_id = 0;
        }

        DebounceCallbackData *callback_data =
            g_malloc(sizeof(DebounceCallbackData));

        callback_data->event = event;
        callback_data->app = app;

        event->debounce_timeout_id = g_timeout_add_full(
            G_PRIORITY_DEFAULT,
            WM_CALLBACK_DEBOUNCE_TIMEOUT,
            events_debounce_callback,
            callback_data,
            g_free
        );
    }

    g_object_unref(parser);
}

/**
 * Click handler to focus the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_focus(const char *id) {
    return wm_click_execute("niri msg action focus-window --id %s", id);
}

/**
 * Click handler to close the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_close(const char *id) {
    return wm_click_execute("niri msg action close-window --id %s", id);
}

/**
 * Click handler to toggle float the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_float(const char *id) {
    return wm_click_execute(
        "niri msg action toggle-window-floating --id %s",
        id
    );
}

/**
 * Called when insertion sorting the windows. Sort from left to right top to
 * bottom
 *
 * @param cur The current window
 * @param prev The previous window
 * @return < 0 if should swap
 */
static int should_swap(WindowManagerWindow *cur, WindowManagerWindow *prev) {
    if (cur->x != prev->x) {
        return cur->x - prev->x;
    }
    return cur->y - prev->y;
}

/**
 * Gets the windows information from the window manager
 *
 * @param app The app instance
 * @param wins An empty array to populate with the window information
 * @return TRUE if successfully fetched windows else FALSE
 */
static gboolean get_windows(WwtApp *app, GPtrArray *wins) {
    WwtConfig *config = wwt_app_get_config(app);
    const char *config_output = wwt_config_get_output(config);

    char *ws_json = cmd_output("niri msg -j workspaces");
    if (!ws_json)
        return FALSE;

    JsonParser *ws_parser = create_json_parser(ws_json);
    if (!ws_parser) {
        g_free(ws_json);
        return FALSE;
    }

    JsonNode *ws_root = json_parser_get_root(ws_parser);
    JsonArray *workspaces = json_node_get_array(ws_root);
    if (!workspaces) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        return FALSE;
    }

    gint64 focused_ws_id = -1;

    guint ws_len = json_array_get_length(workspaces);
    for (guint i = 0; i < ws_len; i++) {
        JsonObject *ws = json_array_get_object_element(workspaces, i);

        const gchar *output = json_object_get_string_member(ws, "output");

        if (config_output && *config_output) {
            // Config output set: find the active (is_active) workspace on that
            // monitor
            if (!output || strcmp(output, config_output) != 0)
                continue;

            gboolean is_active =
                json_object_get_boolean_member(ws, "is_active");
            if (!is_active)
                continue;
        } else {
            // No config output: find the globally focused workspace
            gboolean is_focused =
                json_object_get_boolean_member(ws, "is_focused");
            if (!is_focused)
                continue;
        }

        focused_ws_id = json_object_get_int_member(ws, "id");

        break;
    }

    if (focused_ws_id == -1) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        return FALSE;
    }

    char *win_json = cmd_output("niri msg -j windows");
    if (!win_json) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        return FALSE;
    }

    JsonParser *win_parser = create_json_parser(win_json);
    if (!win_parser) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        g_free(win_json);
        return FALSE;
    }

    JsonNode *win_root = json_parser_get_root(win_parser);
    JsonArray *windows = json_node_get_array(win_root);
    if (!windows) {
        g_object_unref(win_parser);
        g_object_unref(ws_parser);
        g_free(win_json);
        g_free(ws_json);
        return FALSE;
    }

    guint win_len = json_array_get_length(windows);
    for (guint i = 0; i < win_len; i++) {
        JsonObject *win = json_array_get_object_element(windows, i);

        gint64 ws_id = json_object_get_int_member(win, "workspace_id");
        if (ws_id != focused_ws_id) {
            continue;
        }

        gint64 id = json_object_get_int_member(win, "id");
        const gchar *title = json_object_get_string_member(win, "title");
        const gchar *app_id = json_object_get_string_member(win, "app_id");
        gboolean is_focused = json_object_get_boolean_member(win, "is_focused");

        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%" G_GINT64_FORMAT, id);

        gint x = 0, y = 0;
        JsonNode *layout_node = json_object_get_member(win, "layout");

        if (layout_node && !json_node_is_null(layout_node)) {
            JsonObject *layout = json_node_get_object(layout_node);
            JsonNode *pos_node =
                json_object_get_member(layout, "pos_in_scrolling_layout");

            if (pos_node && !json_node_is_null(pos_node)) {
                JsonArray *pos = json_node_get_array(pos_node);

                if (pos && json_array_get_length(pos) >= 2) {
                    x = (gint)json_array_get_int_element(pos, 0);
                    y = (gint)json_array_get_int_element(pos, 1);
                }
            }
        }

        WindowManagerWindow *wmwin =
            wm_win_create(id_str, title, app_id, (int)is_focused, x, y);

        g_ptr_array_add(wins, wmwin);

        gint j = wins->len - 1;
        while (j > 0) {
            WindowManagerWindow *cur = g_ptr_array_index(wins, j);
            WindowManagerWindow *prev = g_ptr_array_index(wins, j - 1);

            if (should_swap(cur, prev) < 0) {
                g_ptr_array_index(wins, j - 1) = cur;
                g_ptr_array_index(wins, j) = prev;
                j--;
            } else {
                break;
            }
        }
    }

    g_object_unref(win_parser);
    g_object_unref(ws_parser);
    g_free(win_json);
    g_free(ws_json);
    return TRUE;
}

/**
 * Create the window manager spec
 *
 * @return (transfer full): The fully created window manager spec
 */
WindowManagerSpec *window_manager_spec_create_niri() {
    WindowManagerSpec *spec = g_malloc(sizeof(WindowManagerSpec));

    spec->events_constructor = events_constructor;
    spec->events_destructor = events_destructor;
    spec->events_reader = events_reader;
    spec->events_callback = events_callback;
    spec->get_windows = get_windows;
    spec->window_focus = window_focus;
    spec->window_close = window_close;
    spec->window_float = window_float;

    return spec;
}
