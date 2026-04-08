#include "config.h"
#include "core/window_manager.h"
#include "json-glib/json-glib.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

struct _WwtConfig {
    GObject parent;

    WindowManagerId window_manager_id;
    gboolean show_title;
    gboolean show_icon;
    int max_tabs;
    int title_max_chars;
    char *output;
    gfloat text_align;
};

G_DEFINE_TYPE(WwtConfig, wwt_config, G_TYPE_OBJECT);

/**
 * Really just for debugging but can be helpful
 *
 * @param self
 */
void wwt_config_log_entries(WwtConfig *self) {
    printf("Config Entries\n");
    printf("output: %s\n", self->output);
    printf("max_tabs: %d\n", self->max_tabs);
    printf("window_manager_id: %d\n", self->window_manager_id);
    printf("show_title: %d\n", self->show_title);
    printf("show_icon: %d\n", self->show_icon);
    printf("title_max_chars: %d\n", self->title_max_chars);
}

/**
 * Gets the window manager id value
 *
 * @param self
 * @return The window manager id value
 */
WindowManagerId wwt_config_get_wm_id(WwtConfig *self) {
    return self->window_manager_id;
}

/**
 * Gets the output value
 *
 * @param self
 * @return The output
 */
const char *wwt_config_get_output(WwtConfig *self) {
    return self->output;
}

/**
 * Gets the show_title value
 *
 * @param self
 * @return The show_title value
 */
gboolean wwt_config_get_show_title(WwtConfig *self) {
    return self->show_title;
}

/**
 * Gets the show_icon value
 *
 * @param self
 * @return The show_icon value
 */
gboolean wwt_config_get_show_icon(WwtConfig *self) {
    return self->show_icon;
}

/**
 * Gets the title_max_chars value
 *
 * @param self
 * @return The title_max_chars value
 */
int wwt_config_get_title_max_chars(WwtConfig *self) {
    return self->title_max_chars;
}

/**
 * Gets the max_tabs value
 *
 * @param self
 * @return The max_tabs value
 */
int wwt_config_get_max_tabs(WwtConfig *self) {
    return self->max_tabs;
}

/**
 * Gets the text_align value
 *
 * @param self
 * @return The text_align value
 */
gfloat wwt_config_get_text_align(WwtConfig *self) {
    return self->text_align;
}

/**
 * Parse the config and set up user options
 *
 * @param config_entries The configuration entries from the module
 * @param config_entries_len The len of config entries
 */
static void parse_config_entries(
    WwtConfig *self,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    for (size_t i = 0; i < config_entries_len; i++) {
        const char *key = config_entries[i].key;
        const char *value = config_entries[i].value;

        JsonParser *parser = create_json_parser(value);

        if (!parser) {
            continue;
        }

        JsonNode *node = json_parser_get_root(parser);

        if (strcmp("output", key) == 0) {
            const char *output = json_node_get_string(node);
            self->output = g_strdup(output);
        }

        if (strcmp("window_manager", key) == 0) {
            const char *window_manager = json_node_get_string(node);

            if (strcmp("sway", window_manager) == 0) {
                self->window_manager_id = WM_ID_SWAY;
            } else if (strcmp("hyprland", window_manager) == 0) {
                self->window_manager_id = WM_ID_HYPRLAND;
            } else if (strcmp("niri", window_manager) == 0) {
                self->window_manager_id = WM_ID_NIRI;
            } else {
                self->window_manager_id = WM_ID_UNSUPPORTED;
            }
        }

        if (strcmp("show_title", key) == 0) {
            int show_title = json_node_get_boolean(node);

            if (show_title) {
                self->show_title = TRUE;
            } else {
                self->show_title = FALSE;
            }
        }

        if (strcmp("show_icon", key) == 0) {
            int show_icon = json_node_get_boolean(node);

            if (show_icon) {
                self->show_title = TRUE;
            } else {
                self->show_icon = FALSE;
            }
        }

        if (strcmp("max_tabs", config_entries[i].key) == 0) {
            int max_tabs = json_node_get_int(node);

            if (max_tabs > 0) {
                self->max_tabs = max_tabs;
            }
        }

        if (strcmp("title_max_chars", config_entries[i].key) == 0) {
            int title_max_chars = json_node_get_int(node);

            if (title_max_chars > 3) {
                self->title_max_chars = title_max_chars;
            }
        }

        if (strcmp("text_align", config_entries[i].key) == 0) {
            const char *text_align = json_node_get_string(node);

            if (strcmp("left", text_align) == 0) {
                self->text_align = TAB_TEXT_ALIGN_LEFT;
            } else if (strcmp("right", text_align) == 0) {
                self->text_align = TAB_TEXT_ALIGN_RIGHT;
            } else {
                self->text_align = TAB_TEXT_ALIGN_CENTER;
            }
        }

        g_object_unref(parser);
    }
}

/**
 * Handles obj disposal
 *
 * @param obj The obj struct
 */
static void dispose(GObject *obj) {
    WwtConfig *self = WWT_CONFIG(obj);

    if (self->output) {
        g_free(self->output);
        self->output = NULL;
    }

    G_OBJECT_CLASS(wwt_config_parent_class)->dispose(obj);
}

/**
 * Handles obj finalization
 *
 * @param obj The obj struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_config_parent_class)->finalize(obj);
}

/**
 * Initialize the config
 *
 * @param self
 */
static void wwt_config_init(WwtConfig *self) {
    // Set default config options
    self->window_manager_id = WM_ID_UNSUPPORTED;
    self->output = NULL;
    self->show_icon = TRUE;
    self->show_title = FALSE;
    self->title_max_chars = -1;
    self->max_tabs = -1;
    self->text_align = TAB_TEXT_ALIGN_CENTER;
}

/**
 * Initialized the class instance and methods
 *
 * @param class the class instance
 */
static void wwt_config_class_init(WwtConfigClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
    G_OBJECT_CLASS(klass)->finalize = finalize;
}

/**
 * Creates a new instance of the config
 *
 * @param config_entries The configuration entries from the module
 * @param config_entries_len The len of config entries
 * @return The fully created config instance
 */
WwtConfig *wwt_config_new(
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    WwtConfig *self = g_object_new(WWT_CONFIG_TYPE, NULL);

    parse_config_entries(self, config_entries, config_entries_len);

    return self;
}
