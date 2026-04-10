#include "hyprland.h"
#include "common.h"
#include "core/utils.h"
#include "core/window_manager_data.h"
#include "core/window_manager_events.h"
#include "core/window_manager_spec.h"
#include "glib.h"
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
 * Checks if the event is an appropriate event to fire on
 *
 * @param event The event to check
 */
static gboolean events_validator(WindowManagerEvent *event) {
    if (strncmp(event->msg, "windowtitle>>", strlen("windowtitle>>")) == 0 ||
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
 * Called when sorting the windows. Sort from left to right top to
 * bottom
 *
 * @param cur The current window
 * @param prev The previous window
 * @return < 0 if should swap
 */
static int should_swap(gconstpointer a, gconstpointer b) {
    const WindowManagerWindow *cur = *(const WindowManagerWindow **)a;
    const WindowManagerWindow *prev = *(const WindowManagerWindow **)b;

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
static WindowManagerData *data_getter() {
    WindowManagerData *wm_data = window_manager_data_create();

    char *batch_json = cmd_output("hyprctl --batch \"clients; monitors\" -j");
    if (!batch_json) {
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    char *split = strstr(batch_json, "\n\n");
    if (!split) {
        g_free(batch_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    *split = '\0';
    char *clients_json = batch_json;
    char *monitors_json = split + 2;

    JsonParser *monitors_parser = create_json_parser(monitors_json);
    if (!monitors_parser) {
        g_free(batch_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    JsonNode *monitors_root = json_parser_get_root(monitors_parser);
    JsonArray *monitors = json_node_get_array(monitors_root);
    guint monitors_len = json_array_get_length(monitors);

    for (guint i = 0; i < monitors_len; i++) {
        JsonObject *monitor = json_array_get_object_element(monitors, i);
        const gchar *name = json_object_get_string_member(monitor, "name");
        gboolean focused = json_object_get_boolean_member(monitor, "focused");
        JsonObject *active_ws =
            json_object_get_object_member(monitor, "activeWorkspace");
        gint ws_id = json_object_get_int_member(active_ws, "id");

        window_manager_data_workspace_create(wm_data, ws_id, focused, name);
    }
    g_object_unref(monitors_parser);

    JsonParser *clients_parser = create_json_parser(clients_json);
    if (!clients_parser) {
        g_free(batch_json);
        window_manager_data_destroy(wm_data);
        return NULL;
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

        const gchar *title = json_object_get_string_member(client, "title");
        const gchar *class = json_object_get_string_member(client, "class");
        const gchar *address = json_object_get_string_member(client, "address");
        gint64 focusHistoryID =
            json_object_get_int_member(client, "focusHistoryID");
        gint64 ws_id = json_object_get_int_member(ws, "id");
        gboolean floating = json_object_get_boolean_member(ws, "floating");

        JsonArray *at = json_object_get_array_member(client, "at");
        gint64 x = json_array_get_int_element(at, 0);
        gint64 y = json_array_get_int_element(at, 1);

        window_manager_data_window_create(
            wm_data,
            address,
            title,
            class,
            ws_id,
            focusHistoryID == 0 ? 1 : 0,
            floating,
            x,
            y
        );
    }

    g_object_unref(clients_parser);
    g_free(batch_json);

    window_manager_data_sort_windows(wm_data, should_swap);

    return wm_data;
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
    spec->events_validator = events_validator;
    spec->data_getter = data_getter;
    spec->window_focus = window_focus;
    spec->window_close = window_close;
    spec->window_float = window_float;

    return spec;
}
