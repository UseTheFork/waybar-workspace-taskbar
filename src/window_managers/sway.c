#include "sway.h"
#include "common.h"
#include "core/app.h"
#include "core/utils.h"
#include "glib.h"
#include "widgets/taskbar.h"
#include <stdio.h>

#define SWAY_MAGIC "i3-ipc"
#define SWAY_MAGIC_LEN 6
#define SWAY_HEADER_LEN (SWAY_MAGIC_LEN + 8)

/**
 * Connect and initialize the socket connection
 *
 * @return The file descriptor
 */
static int events_constructor() {
    const char *socket_path = getenv("SWAYSOCK");

    if (!socket_path) {
        return -1;
    }

    int fd = socket_connect(socket_path);

    if (fd < 0) {
        return fd;
    }

    const char *events_json = "[\"workspace\", \"window\"]";

    uint32_t payload_len = strlen(events_json);
    uint32_t msg_type = 2; // IPC_SUBSCRIBE

    char header[SWAY_HEADER_LEN];
    memcpy(header, SWAY_MAGIC, SWAY_MAGIC_LEN);
    memcpy(header + SWAY_MAGIC_LEN, &payload_len, 4);
    memcpy(header + SWAY_MAGIC_LEN + 4, &msg_type, 4);

    write(fd, header, SWAY_HEADER_LEN);
    write(fd, events_json, payload_len);

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
    char header[SWAY_HEADER_LEN];

    if (fread(header, 1, SWAY_HEADER_LEN, socket_file) != SWAY_HEADER_LEN) {
        return FALSE;
    }

    if (memcmp(header, SWAY_MAGIC, SWAY_MAGIC_LEN) != 0) {
        return FALSE;
    }

    uint32_t payload_len, msg_type;
    memcpy(&payload_len, header + SWAY_MAGIC_LEN, 4);
    memcpy(&msg_type, header + SWAY_MAGIC_LEN + 4, 4);

    if ((size_t)payload_len + 1 > event->msg_size) {
        event->msg = realloc(event->msg, payload_len + 1);
        event->msg_size = payload_len + 1;
    }

    if (fread(event->msg, 1, payload_len, socket_file) != payload_len) {
        return FALSE;
    }

    event->msg[payload_len] = '\0';
    event->msg_len = payload_len;

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
    printf("%s\n", event->msg);

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
static void events_callback(WindowManagerEvent *event, gpointer user_data) {
    WwtApp *app = user_data;

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

/**
 * Click handler to focus the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_focus(const char *id) {
    return wm_click_execute("swaymsg \"[con_id=%s] focus\"", id);
}

/**
 * Click handler to close the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_close(const char *id) {
    return wm_click_execute("swaymsg \"[con_id=%s] kill\"", id);
}

/**
 * Click handler to toggle float the window
 *
 * @param id The window id
 * @return Whether the command was successfully executed
 */
static gboolean window_float(const char *id) {
    return wm_click_execute("swaymsg \"[con_id=%s] floating toggle\"", id);
}

/**
 * Walks the sway tree and finds windows
 *
 * @param wins The windows array
 * @param node The current node to process
 * @param workspace_name The current active workspace name
 */
static void walk_tree(
    GPtrArray *wins,
    JsonObject *node,
    const char *workspace_name
) {
    if (!node) {
        return;
    }

    const char *type = json_object_get_string_member(node, "type");

    gint64 pid = 0;
    if (json_object_has_member(node, "pid")) {
        pid = json_object_get_int_member(node, "pid");
    }

    // It's a real window if type=con and pid exists and nodes is empty
    if (type && pid &&
        (strcmp(type, "con") == 0 || strcmp(type, "floating_con") == 0)) {

        gint64 id = json_object_get_int_member(node, "id");
        const char *name = json_object_get_string_member(node, "name");
        const char *app_id = json_object_get_string_member(node, "app_id");
        gboolean focused = json_object_get_boolean_member(node, "focused");

        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%" G_GINT64_FORMAT, id);

        gint x = 0;
        gint y = 0;

        JsonObject *window_rect =
            json_object_get_object_member(node, "window_rect");

        if (window_rect) {
            x = json_object_get_int_member(window_rect, "x");
            y = json_object_get_int_member(window_rect, "y");
        }

        WindowManagerWindow *win =
            wm_win_create(id_str, name, app_id, focused, x, y);

        g_ptr_array_add(wins, win);
    }

    JsonArray *nodes = json_object_get_array_member(node, "nodes");

    if (nodes) {
        guint nodes_len = json_array_get_length(nodes);
        for (guint i = 0; i < nodes_len; i++) {
            JsonObject *child = json_array_get_object_element(nodes, i);
            walk_tree(wins, child, workspace_name);
        }
    }

    JsonArray *floating_nodes =
        json_object_get_array_member(node, "floating_nodes");
    if (floating_nodes) {
        guint floating_len = json_array_get_length(floating_nodes);

        for (guint i = 0; i < floating_len; i++) {
            JsonObject *child =
                json_array_get_object_element(floating_nodes, i);
            walk_tree(wins, child, workspace_name);
        }
    }
}

/**
 * Gets the windows information from the window manager
 *
 * @param app The app instance
 * @param wins An empty array to populate with the window information
 * @return TRUE if successfully fetched windows else FALSE
 */
static gboolean get_windows(WwtApp *app, GPtrArray *wins) {
    const char *monitor_name = "HDMI-A-1";

    char *json_str = cmd_output("swaymsg -t get_tree");

    if (!json_str) {
        return FALSE;
    }

    JsonParser *parser = create_json_parser(json_str);

    if (!parser) {
        g_free(json_str);

        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);
    JsonArray *outputs = json_object_get_array_member(root_obj, "nodes");

    if (!outputs) {
        g_object_unref(parser);
        g_free(json_str);
        return FALSE;
    }

    guint outputs_len = json_array_get_length(outputs);

    for (guint i = 0; i < outputs_len; i++) {
        JsonObject *output = json_array_get_object_element(outputs, i);
        const gchar *name = json_object_get_string_member(output, "name");

        if (!name || strcmp(name, monitor_name) != 0) {
            continue;
        }

        const gchar *current_ws =
            json_object_get_string_member(output, "current_workspace");

        if (!current_ws) {
            continue;
        }

        JsonArray *nodes = json_object_get_array_member(output, "nodes");

        if (!nodes) {
            continue;
        }

        guint nodes_len = json_array_get_length(nodes);

        for (guint j = 0; j < nodes_len; j++) {
            JsonObject *ws = json_array_get_object_element(nodes, j);
            const gchar *ws_name = json_object_get_string_member(ws, "name");

            if (ws_name && strcmp(ws_name, current_ws) == 0) {
                walk_tree(wins, ws, ws_name);
            }
        }
    }

    g_object_unref(parser);
    g_free(json_str);

    return TRUE;
}

/**
 * Create the window manager spec
 *
 * @return (transfer full): The fully created window manager spec
 */
WindowManagerSpec *window_manager_spec_create_sway() {
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
