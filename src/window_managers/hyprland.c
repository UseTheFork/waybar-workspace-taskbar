#include "hyprland.h"
#include "common.h"
#include "core/app.h"
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
    const char *his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");

    if (!his || !xdg_runtime) {
        fprintf(stderr, "Missing environment variables.\n");
        return -1;
    }

    char socket_path[512];
    snprintf(
        socket_path,
        sizeof(socket_path),
        "%s/hypr/%s/.socket2.sock",
        xdg_runtime,
        his
    );

    int fd = socket_connect(socket_path);

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
    if (strncmp(event->msg, "windowtitle>>", strlen("windowtitle>>")) == 0 ||
        strncmp(event->msg, "workspace>>", strlen("workspace>>")) == 0 ||
        strncmp(event->msg, "activewindow>>", strlen("activewindow>>")) == 0 ||
        strncmp(event->msg, "closewindow>>", strlen("closewindow>>")) == 0 ||
        strncmp(event->msg, "openwindow>>", strlen("openwindow>>")) == 0 ||
        strncmp(event->msg, "activewindow>>", strlen("activewindow>>")) == 0) {

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
}

/**
 * Click handler to focus the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_focus(const char *id) {
    return wm_click_execute("hyprctl dispatch focuswindow address:%s", id);
}

/**
 * Click handler to close the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_close(const char *id) {
    return wm_click_execute("hyprctl dispatch closewindow address:%s", id);
}

/**
 * Click handler to toggle float the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_float(const char *id) {
    return wm_click_execute("hyprctl dispatch togglefloating address:%s", id);
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

    char *batch_json = cmd_output("hyprctl --batch \"clients; monitors\" -j");
    if (!batch_json)
        return FALSE;

    char *split = strstr(batch_json, "\n\n");
    if (!split) {
        g_free(batch_json);
        return FALSE;
    }
    *split = '\0';
    char *clients_json = batch_json;
    char *monitors_json = split + 2;

    JsonParser *monitors_parser = create_json_parser(monitors_json);
    if (!monitors_parser) {
        g_free(batch_json);
        return FALSE;
    }

    JsonNode *monitors_root = json_parser_get_root(monitors_parser);
    JsonArray *monitors = json_node_get_array(monitors_root);
    guint monitors_len = json_array_get_length(monitors);

    gint64 active_workspace_id = -1;
    gchar *workspace_name = NULL;

    for (guint i = 0; i < monitors_len; i++) {
        JsonObject *monitor = json_array_get_object_element(monitors, i);
        const gchar *name = json_object_get_string_member(monitor, "name");

        if (config_output && *config_output) {
            if (!name || strcmp(name, config_output) != 0)
                continue;
        } else {
            gboolean focused =
                json_object_get_boolean_member(monitor, "focused");
            if (!focused)
                continue;
        }

        JsonObject *active_ws =
            json_object_get_object_member(monitor, "activeWorkspace");
        if (active_ws) {
            gint64 ws_id = json_object_get_int_member(active_ws, "id");
            const gchar *ws_name =
                json_object_get_string_member(active_ws, "name");
            if (ws_id && ws_name) {
                active_workspace_id = ws_id;
                workspace_name = g_strdup(ws_name);
            }
        }
        break;
    }
    g_object_unref(monitors_parser);

    if (active_workspace_id == -1) {
        g_free(workspace_name);
        g_free(batch_json);
        return FALSE;
    }

    JsonParser *clients_parser = create_json_parser(clients_json);

    if (!clients_parser) {
        g_free(workspace_name);
        g_free(batch_json);
        return FALSE;
    }

    JsonNode *clients_root = json_parser_get_root(clients_parser);
    JsonArray *clients = json_node_get_array(clients_root);
    guint clients_len = json_array_get_length(clients);

    for (guint i = 0; i < clients_len; i++) {
        JsonObject *client = json_array_get_object_element(clients, i);
        JsonObject *ws = json_object_get_object_member(client, "workspace");

        if (!ws) {
            continue;
        }

        gint64 ws_id = json_object_get_int_member(ws, "id");

        if (ws_id != active_workspace_id) {
            continue;
        }

        const gchar *title = json_object_get_string_member(client, "title");
        const gchar *class = json_object_get_string_member(client, "class");
        const gchar *address = json_object_get_string_member(client, "address");
        gint64 focusHistoryID =
            json_object_get_int_member(client, "focusHistoryID");

        JsonArray *at = json_object_get_array_member(client, "at");
        gint64 x = json_array_get_int_element(at, 0);
        gint64 y = json_array_get_int_element(at, 1);

        WindowManagerWindow *win = wm_win_create(
            address,
            title,
            class,
            focusHistoryID == 0 ? 1 : 0,
            x,
            y
        );

        g_ptr_array_add(wins, win);

        int j = wins->len - 1;
        while (j > 0) {
            WindowManagerWindow *cur = g_ptr_array_index(wins, j);
            WindowManagerWindow *prev = g_ptr_array_index(wins, j - 1);

            if (should_swap(cur, prev) < 0) {
                WindowManagerWindow *tmp = prev;
                g_ptr_array_index(wins, j - 1) = cur;
                g_ptr_array_index(wins, j) = tmp;
                j--;
            } else {
                break;
            }
        }
    }

    g_object_unref(clients_parser);
    g_free(workspace_name);
    g_free(batch_json);

    return TRUE;
}

/**
 * Create the window manager spec
 *
 * @return (transfer full): The fully created window manager spec
 */
WindowManagerSpec *window_manager_spec_create_hyprland() {
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
