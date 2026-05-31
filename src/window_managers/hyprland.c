#include "hyprland.h"
#include "glib.h"
#include "services/window_manager/data.h"
#include "services/window_manager/events.h"
#include "utils/cmd.h"
#include "utils/common.h"
#include <stdio.h>

/**
 * Connect and initialize the socket connection
 *
 * @return The file descriptor
 */
static int events_constructor() {
    const char *his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");

    if(!his || !xdg_runtime) {
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
    if(getline(&event->msg, &event->msg_size, socket_file) <= 0) {
        clearerr(socket_file);
        return FALSE;
    }

    event->msg_len = strlen(event->msg);

    return TRUE;
}

/**
 * Checks if the event is an appropriate event to fire on
 *
 * @param event The event to check
 * @return TRUE if should fire events else FALSE
 */
static gboolean events_validator(WindowManagerEvent *event) {
    if(strncmp(event->msg, "windowtitle>>", strlen("windowtitle>>")) == 0 ||
        strncmp(event->msg, "workspace>>", strlen("workspace>>")) == 0 ||
        strncmp(event->msg, "activewindow>>", strlen("activewindow>>")) == 0 ||
        strncmp(event->msg, "closewindow>>", strlen("closewindow>>")) == 0 ||
        strncmp(event->msg, "openwindow>>", strlen("openwindow>>")) == 0 ||
        strncmp(event->msg, "activewindow>>", strlen("activewindow>>")) == 0) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Called when sorting the windows. Sort from left to right top to
 * bottom. Sort all floating windows to end.
 *
 * @param a The current window
 * @param b The next window
 * @return < 0 if should swap
 */
static int window_sort(gconstpointer a, gconstpointer b) {
    const WindowManagerDataWindow *cur = *(const WindowManagerDataWindow **)a;
    const WindowManagerDataWindow *next = *(const WindowManagerDataWindow **)b;

    if(cur->floating != next->floating) {
        return cur->floating - next->floating;
    }

    if(cur->x != next->x) {
        return cur->x - next->x;
    }

    if(cur->y != next->y) {
        return cur->y - next->y;
    }

    // Fallback for monocle layout
    return cur->sortable - next->sortable;
}

/**
 * Gets the windows information from the window manager and populates the window
 * manager data
 *
 * @param wm_data The window manager data to populate
 */
static void data_fetcher(WwtWindowManagerData *wm_data) {
    g_autofree char *batch_json =
        cmd_run_output("hyprctl --batch \"monitors; clients\" -j");

    if(!batch_json) {
        return;
    }

    char *split = strstr(batch_json, "\n\n");
    if(!split) {
        return;
    }
    *split = '\0';

    char *monitors_json = batch_json;
    char *clients_json = split + 2;

    g_autoptr(JsonParser) monitors_parser = create_json_parser(monitors_json);
    if(!monitors_parser) {
        return;
    }

    JsonNode *monitors_root = json_parser_get_root(monitors_parser);
    JsonArray *monitors = json_node_get_array(monitors_root);
    guint monitors_len = json_array_get_length(monitors);

    for(guint i = 0; i < monitors_len; i++) {
        JsonObject *monitor = json_array_get_object_element(monitors, i);
        const gchar *name = json_object_get_string_member(monitor, "name");
        gboolean focused = json_object_get_boolean_member(monitor, "focused");
        JsonObject *active_ws =
            json_object_get_object_member(monitor, "activeWorkspace");
        gint ws_id = json_object_get_int_member(active_ws, "id");

        wwt_window_manager_data_workspace_add(wm_data, ws_id, focused, name);
    }

    g_autoptr(JsonParser) clients_parser = create_json_parser(clients_json);
    if(!clients_parser) {
        return;
    }

    JsonNode *clients_root = json_parser_get_root(clients_parser);
    JsonArray *clients = json_node_get_array(clients_root);
    guint clients_len = json_array_get_length(clients);

    for(guint i = 0; i < clients_len; i++) {
        JsonObject *client = json_array_get_object_element(clients, i);
        JsonObject *ws = json_object_get_object_member(client, "workspace");

        if(!ws) {
            continue;
        }

        const gchar *title = json_object_get_string_member(client, "title");
        const gchar *class = json_object_get_string_member(client, "class");
        const gchar *address = json_object_get_string_member(client, "address");
        gint64 focusHistoryID =
            json_object_get_int_member(client, "focusHistoryID");
        gint64 ws_id = json_object_get_int_member(ws, "id");
        gboolean floating = json_object_get_boolean_member(client, "floating");
        const gchar *stable_id =
            json_object_get_string_member(client, "stableId");

        JsonArray *at = json_object_get_array_member(client, "at");
        gint64 x = json_array_get_int_element(at, 0);
        gint64 y = json_array_get_int_element(at, 1);

        wwt_window_manager_data_window_add(
            wm_data,
            address,
            title,
            class,
            ws_id,
            focusHistoryID == 0 ? 1 : 0,
            floating,
            FALSE,
            x,
            y,
            (int)strtoul(stable_id, NULL, 16)
        );
    }

    wwt_window_manager_data_sort_windows(wm_data, window_sort);
}

/**
 * Creates the window manager spec
 *
 * @return The spec factory
 */
WindowManagerSpecFactory window_manager_spec_factory_hyprland() {
    return (WindowManagerSpecFactory){
        .events_constructor = events_constructor,
        .events_destructor = events_destructor,
        .events_reader = events_reader,
        .events_validator = events_validator,
        .data_fetcher = data_fetcher,
    };
}
