#include "sway.h"
#include "services/window_manager/data.h"
#include "services/window_manager/events.h"
#include "utils/common.h"
#include "utils/socket.h"
#include <stdio.h>
#include <string.h>

// Sway header
#define SWAY_MAGIC "i3-ipc"
#define SWAY_HEADER_MAGIC_SIZE 6
#define SWAY_HEADER_PAYLOAD_LEN_SIZE 4
#define SWAY_HEADER_PAYLOAD_TYPE_SIZE 4
#define SWAY_HEADER_SIZE                                     \
    (SWAY_HEADER_MAGIC_SIZE + SWAY_HEADER_PAYLOAD_LEN_SIZE + \
        SWAY_HEADER_PAYLOAD_TYPE_SIZE)

#define SWAY_PAYLOAD_TYPE_SUBSCRIBE 2
#define SWAY_PAYLOAD_TYPE_GET_TREE 4

/**
 * Gets the sway socket path
 *
 * @return The socket path
 */
static char *get_socket_path() {
    return getenv("SWAYSOCK");
}

/**
 * Writes to the sway ipc header
 *
 * @param fd The ipc file descriptor
 * @param payload_len The length of the payload
 * @param payload_type The type of payload
 * @return TRUE if successful else FALSE
 */
static gboolean sway_header_write(
    int fd,
    uint32_t payload_len,
    uint32_t payload_type
) {
    char header[SWAY_HEADER_SIZE];

    memcpy(header, SWAY_MAGIC, SWAY_HEADER_MAGIC_SIZE);
    memcpy(
        header + SWAY_HEADER_MAGIC_SIZE,
        &payload_len,
        SWAY_HEADER_PAYLOAD_LEN_SIZE
    );
    memcpy(
        header + SWAY_HEADER_MAGIC_SIZE + SWAY_HEADER_PAYLOAD_LEN_SIZE,
        &payload_type,
        SWAY_HEADER_PAYLOAD_TYPE_SIZE
    );

    if(write(fd, header, SWAY_HEADER_SIZE) != SWAY_HEADER_SIZE) {
        return FALSE;
    }

    return TRUE;
}

/**
 * Reads the sway ipc header
 *
 * @param fd The ipc file descriptor
 * @param payload_len The length of the payload
 * @param payload_type The type of payload
 * @return TRUE if successful else FALSE
 */
static gboolean sway_header_read(
    int fd,
    uint32_t *payload_len,
    uint32_t *msg_type
) {
    char header[SWAY_HEADER_SIZE];

    if(read(fd, header, SWAY_HEADER_SIZE) != SWAY_HEADER_SIZE) {
        return FALSE;
    }

    memcpy(
        payload_len,
        header + SWAY_HEADER_MAGIC_SIZE,
        SWAY_HEADER_PAYLOAD_LEN_SIZE
    );
    memcpy(
        msg_type,
        header + SWAY_HEADER_MAGIC_SIZE + SWAY_HEADER_PAYLOAD_LEN_SIZE,
        SWAY_HEADER_PAYLOAD_TYPE_SIZE
    );

    return TRUE;
}

/**
 * Featches data from the ipc socket
 *
 * @param payload_type The sway message type
 * @param cmd The payload to send to the sway ipc socket
 * @return The ipc response
 */
static char *ipc_fetch(uint32_t payload_type, const char *cmd) {
    uint32_t input_payload_len = cmd ? strlen(cmd) : 0;
    const char *socket_path = get_socket_path();

    if(!socket_path) {
        return NULL;
    }

    int fd = socket_connect(socket_path);
    gboolean write_ok = sway_header_write(fd, input_payload_len, payload_type);

    if(!write_ok) {
        close(fd);
        return NULL;
    }

    uint32_t output_payload_len;
    uint32_t output_payload_type;

    gboolean read_ok =
        sway_header_read(fd, &output_payload_len, &output_payload_type);

    if(!read_ok) {
        close(fd);
        return NULL;
    }

    char *data = socket_read_len(fd, output_payload_len);
    close(fd);

    return data;
}

/**
 * Connect and initialize the socket connection
 *
 * @return The file descriptor
 */
static int events_constructor() {
    const char *socket_path = get_socket_path();

    if(!socket_path) {
        return -1;
    }

    int fd = socket_connect(socket_path);

    if(fd < 0) {
        return fd;
    }

    const char *events_json = "[\"workspace\", \"window\"]";
    uint32_t payload_len = strlen(events_json);
    uint32_t payload_type = SWAY_PAYLOAD_TYPE_SUBSCRIBE;

    // header
    // write(fd, SWAY_MAGIC, SWAY_HEADER_MAGIC_SIZE);
    // write(fd, &payload_len, SWAY_HEADER_PAYLOAD_LEN_SIZE);
    // write(fd, &payload_type, SWAY_HEADER_PAYLOAD_TYPE_SIZE);
    sway_header_write(fd, payload_len, payload_type);

    // command
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
    char header[SWAY_HEADER_SIZE];

    if(fread(header, 1, SWAY_HEADER_SIZE, socket_file) != SWAY_HEADER_SIZE) {
        return FALSE;
    }

    if(memcmp(header, SWAY_MAGIC, SWAY_HEADER_MAGIC_SIZE) != 0) {
        return FALSE;
    }

    uint32_t payload_len, msg_type;
    memcpy(&payload_len, header + SWAY_HEADER_MAGIC_SIZE, 4);
    memcpy(&msg_type, header + SWAY_HEADER_MAGIC_SIZE + 4, 4);

    if((size_t)payload_len + 1 > event->msg_size) {
        event->msg = realloc(event->msg, payload_len + 1);
        event->msg_size = payload_len + 1;
    }

    if(fread(event->msg, 1, payload_len, socket_file) != payload_len) {
        return FALSE;
    }

    event->msg[payload_len] = '\0';
    event->msg_len = payload_len;

    return TRUE;
}

/**
 * Checks if the event is an appropriate event to fire on
 *
 * @param event The event to check
 * @return TRUE if should fire events else FALSE
 */
static gboolean events_validator(WindowManagerEvent *event) {
    g_autoptr(JsonParser) parser = create_json_parser(event->msg);
    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);

    if(json_object_has_member(root_obj, "change")) {
        const gchar *change = json_object_get_string_member(root_obj, "change");

        if(strcmp("title", change) == 0 || strcmp("focus", change) == 0 ||
            strcmp("new", change) == 0 || strcmp("move", change) == 0 ||
            strcmp("close", change) == 0 || strcmp("empty", change) == 0 ||
            strcmp("floating", change) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Called when sorting the windows. Sort floating windows to end
 *
 * @param a The current window
 * @param b The next window
 * @return < 0 if should swap
 */
static int window_sort(gconstpointer a, gconstpointer b) {
    const WindowManagerDataWindow *cur = *(const WindowManagerDataWindow **)a;
    const WindowManagerDataWindow *next = *(const WindowManagerDataWindow **)b;

    return cur->floating - next->floating;
}

/**
 * Walks the sway tree and finds windows
 *
 * @param wins The windows array
 * @param node The current node to process
 * @param workspace_name The current active workspace name
 */
static void walk_tree(
    WwtWindowManagerData *wm_data,
    JsonObject *node,
    int workspace_id
) {
    if(!node) {
        return;
    }

    const char *type = json_object_get_string_member(node, "type");

    gint64 pid = 0;
    if(json_object_has_member(node, "pid")) {
        pid = json_object_get_int_member(node, "pid");
    }

    // It's a real window if type=con and pid exists and nodes is empty
    if(type && pid &&
        (strcmp(type, "con") == 0 || strcmp(type, "floating_con") == 0)) {
        gint64 id = json_object_get_int_member(node, "id");
        const gchar *name = json_object_get_string_member(node, "name");
        const gchar *app_id = json_object_get_string_member(node, "app_id");
        gboolean focused = json_object_get_boolean_member(node, "focused");
        gboolean urgent = json_object_get_boolean_member(node, "urgent");

        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%" G_GINT64_FORMAT, id);

        gint x = 0;
        gint y = 0;

        JsonObject *window_rect =
            json_object_get_object_member(node, "window_rect");

        if(window_rect) {
            x = json_object_get_int_member(window_rect, "x");
            y = json_object_get_int_member(window_rect, "y");
        }

        wwt_window_manager_data_window_add(
            wm_data,
            id_str,
            name,
            app_id,
            workspace_id,
            focused,
            strcmp("floating_con", type) == 0,
            urgent,
            x,
            y,
            0
        );

        if(focused) {
            wwt_window_manager_data_set_focused_workspace(
                wm_data,
                workspace_id
            );
        }
    }

    JsonArray *nodes = json_object_get_array_member(node, "nodes");

    if(nodes) {
        guint nodes_len = json_array_get_length(nodes);
        for(guint i = 0; i < nodes_len; i++) {
            JsonObject *child = json_array_get_object_element(nodes, i);
            walk_tree(wm_data, child, workspace_id);
        }
    }

    JsonArray *floating_nodes =
        json_object_get_array_member(node, "floating_nodes");

    if(floating_nodes) {
        guint floating_len = json_array_get_length(floating_nodes);

        for(guint i = 0; i < floating_len; i++) {
            JsonObject *child =
                json_array_get_object_element(floating_nodes, i);
            walk_tree(wm_data, child, workspace_id);
        }
    }
}

/**
 * Gets the windows information from the window manager
 *
 * @return (transfer full): Populated window manager data or NULL on error (free
 * with window_manager_data_destroy)
 */
static void data_fetcher(WwtWindowManagerData *wm_data) {
    g_autofree char *json_str = ipc_fetch(SWAY_PAYLOAD_TYPE_GET_TREE, NULL);
    if(!json_str) {
        return;
    }

    g_autoptr(JsonParser) parser = create_json_parser(json_str);
    if(!parser) {
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);
    JsonArray *outputs = json_object_get_array_member(root_obj, "nodes");
    if(!outputs) {
        return;
    }

    guint outputs_len = json_array_get_length(outputs);
    for(guint i = 0; i < outputs_len; i++) {
        JsonObject *output = json_array_get_object_element(outputs, i);

        if(!json_object_has_member(output, "current_workspace")) {
            continue;
        }

        const gchar *current_ws =
            json_object_get_string_member(output, "current_workspace");

        const gchar *output_name =
            json_object_get_string_member(output, "name");
        if(!output_name) {
            continue;
        }

        JsonArray *nodes = json_object_get_array_member(output, "nodes");
        if(!nodes) {
            continue;
        }

        guint nodes_len = json_array_get_length(nodes);
        for(guint j = 0; j < nodes_len; j++) {
            JsonObject *ws = json_array_get_object_element(nodes, j);
            const gchar *ws_name = json_object_get_string_member(ws, "name");

            if(ws_name && strcmp(ws_name, current_ws) != 0) {
                continue;
            }

            gint64 workspace_id = json_object_get_int_member(ws, "id");
            gboolean focused = json_object_get_boolean_member(ws, "focused");

            wwt_window_manager_data_workspace_add(
                wm_data,
                workspace_id,
                focused,
                output_name
            );

            walk_tree(wm_data, ws, workspace_id);
        }
    }

    wwt_window_manager_data_sort_windows(wm_data, window_sort);
}

/**
 * Create the window manager spec
 *
 * @return (transfer full): The fully created window manager spec
 */
WindowManagerSpecFactory window_manager_spec_factory_sway() {
    return (WindowManagerSpecFactory){
        .events_constructor = events_constructor,
        .events_destructor = events_destructor,
        .events_reader = events_reader,
        .events_validator = events_validator,
        .data_fetcher = data_fetcher,
    };
}
