#include "niri.h"
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
    const char *socket_path = getenv("NIRI_SOCKET");

    if(!socket_path) {
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
    JsonParser *parser = create_json_parser(event->msg);
    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);

    if(json_object_has_member(root_obj, "WindowOpenedOrChanged") ||
        json_object_has_member(root_obj, "WindowClosed") ||
        json_object_has_member(root_obj, "WindowLayoutsChanged") ||
        json_object_has_member(root_obj, "WorkspacesChanged") ||
        json_object_has_member(root_obj, "WindowFocusChanged") ||
        json_object_has_member(root_obj, "WorkspaceActivated") ||
        json_object_has_member(root_obj, "WorkspaceActiveWindowChanged") ||
        json_object_has_member(root_obj, "WindowUrgencyChanged") ||
        json_object_has_member(root_obj, "OverviewOpenedOrClosed")) {
        g_object_unref(parser);
        return TRUE;
    }

    g_object_unref(parser);
    return FALSE;
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
 * Called when sorting the windows. Sort from left to right top to
 * bottom. Sort all floating windows to end.
 *
 * @param a The current window
 * @param b The next window
 * @return < 0 if should swap
 */
static int window_sort(gconstpointer a, gconstpointer b) {
    const WindowManagerWindow *cur = *(const WindowManagerWindow **)a;
    const WindowManagerWindow *next = *(const WindowManagerWindow **)b;

    if(cur->floating != next->floating) {
        return cur->floating - next->floating;
    }

    if(cur->x != next->x) {
        return cur->x - next->x;
    }

    return cur->y - next->y;
}

/**
 * Gets the window and workspace information from the window manager
 *
 * @return (transfer full): Populated window manager data or NULL on error
 */
static WindowManagerData *data_getter() {
    WindowManagerData *wm_data = window_manager_data_create();

    char *ws_json = cmd_output("niri msg -j workspaces");
    if(!ws_json) {
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    JsonParser *ws_parser = create_json_parser(ws_json);
    if(!ws_parser) {
        g_free(ws_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    JsonNode *ws_root = json_parser_get_root(ws_parser);
    JsonArray *workspaces = json_node_get_array(ws_root);
    if(!workspaces) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    guint ws_len = json_array_get_length(workspaces);
    for(guint i = 0; i < ws_len; i++) {
        JsonObject *ws = json_array_get_object_element(workspaces, i);
        gboolean is_active = json_object_get_boolean_member(ws, "is_active");

        if(!is_active) {
            continue;
        }

        const gchar *output = json_object_get_string_member(ws, "output");
        gboolean is_focused = json_object_get_boolean_member(ws, "is_focused");
        int ws_id = json_object_get_int_member(ws, "id");

        window_manager_data_workspace_create(
            wm_data,
            ws_id,
            is_focused,
            output
        );
    }

    char *win_json = cmd_output("niri msg -j windows");
    if(!win_json) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    JsonParser *win_parser = create_json_parser(win_json);
    if(!win_parser) {
        g_object_unref(ws_parser);
        g_free(ws_json);
        g_free(win_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    JsonNode *win_root = json_parser_get_root(win_parser);
    JsonArray *windows = json_node_get_array(win_root);
    if(!windows) {
        g_object_unref(win_parser);
        g_object_unref(ws_parser);
        g_free(win_json);
        g_free(ws_json);
        window_manager_data_destroy(wm_data);
        return NULL;
    }

    guint win_len = json_array_get_length(windows);
    for(guint i = 0; i < win_len; i++) {
        JsonObject *win = json_array_get_object_element(windows, i);

        gint64 id = json_object_get_int_member(win, "id");
        gint64 ws_id = json_object_get_int_member(win, "workspace_id");
        const gchar *title = json_object_get_string_member(win, "title");
        const gchar *app_id = json_object_get_string_member(win, "app_id");
        gboolean is_focused = json_object_get_boolean_member(win, "is_focused");
        gboolean is_floating =
            json_object_get_boolean_member(win, "is_floating");
        gboolean is_urgent = json_object_get_boolean_member(win, "is_urgent");

        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%" G_GINT64_FORMAT, id);

        gint x = 0, y = 0;
        JsonNode *layout_node = json_object_get_member(win, "layout");

        if(layout_node && !json_node_is_null(layout_node)) {
            JsonObject *layout = json_node_get_object(layout_node);
            JsonNode *pos_node =
                json_object_get_member(layout, "pos_in_scrolling_layout");

            if(pos_node && !json_node_is_null(pos_node)) {
                JsonArray *pos = json_node_get_array(pos_node);

                if(pos && json_array_get_length(pos) >= 2) {
                    x = (gint)json_array_get_int_element(pos, 0);
                    y = (gint)json_array_get_int_element(pos, 1);
                }
            }
        }

        window_manager_data_window_create(
            wm_data,
            id_str,
            title,
            app_id,
            ws_id,
            is_focused,
            is_floating,
            is_urgent,
            x,
            y,
            0
        );
    }

    window_manager_data_sort_windows(wm_data, window_sort);
    g_object_unref(win_parser);
    g_object_unref(ws_parser);
    g_free(win_json);
    g_free(ws_json);

    return wm_data;
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
    spec->events_validator = events_validator;
    spec->data_getter = data_getter;
    spec->window_focus = window_focus;
    spec->window_close = window_close;
    spec->window_float = window_float;

    return spec;
}
