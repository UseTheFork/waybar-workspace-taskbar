#include "config.h"
#include "core/app.h"
#include "glib.h"
#include "json-glib/json-glib.h"
#include "services/window_manager_spec.h"
#include "utils/common.h"
#include "widgets/navigation_btn.h"
#include <stdlib.h>
#include <string.h>

struct _WwtConfig {
    GObject parent_instance;

    WindowManagerId window_manager_id;
    gboolean show_title;
    gboolean show_icon;
    gboolean show_tooltip;
    int max_tabs;
    int title_max_chars;
    char *output;
    gfloat text_align;
    int icon_size;
    NavigationBtnDisplayType show_navigation_btns;
    NavigationBtnPos navigation_btn_pos;
    gchar *navigation_btn_prev_label;
    gchar *navigation_btn_next_label;
    gchar *on_click;
    gchar *on_click_middle;
    gchar *on_click_right;
    gchar *navigation_btn_on_click;
};

G_DEFINE_TYPE(WwtConfig, wwt_config, G_TYPE_OBJECT);

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
 * Gets the show_tooltip value
 *
 * @param self
 * @return The show_tooltip value
 */
int wwt_config_get_show_tooltip(WwtConfig *self) {
    return self->show_tooltip;
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
 * Gets the icon_size value
 *
 * @param self
 * @return The icons_size value
 */
int wwt_config_get_icon_size(WwtConfig *self) {
    return self->icon_size;
}

/**
 * Gets the show_navigation_btns value
 *
 * @param self
 * @return The show_navigation_btns value
 */
gboolean wwt_config_get_show_navigation_btns(WwtConfig *self) {
    return self->show_navigation_btns;
}

/**
 * Gets the navigation_btn_prev_label value
 *
 * @param self
 * @return The navigation_btn_prev_label value
 */
gchar *wwt_config_get_navigation_btn_prev_label(WwtConfig *self) {
    return self->navigation_btn_prev_label;
}

/**
 * Gets the navigation_btn_next_label value
 *
 * @param self
 * @return The navigation_btn_next_label value
 */
gchar *wwt_config_get_navigation_btn_next_label(WwtConfig *self) {
    return self->navigation_btn_next_label;
}

/**
 * Gets the navigation_btn_pos value
 *
 * @param self
 * @return The navigation_btn_pos value
 */
NavigationBtnPos wwt_config_get_navigation_btn_pos(WwtConfig *self) {
    return self->navigation_btn_pos;
}

/**
 * Gets the on_click value
 *
 * @param self
 * @return The on_click value
 */
gchar *wwt_config_get_on_click(WwtConfig *self) {
    return self->on_click;
}

/**
 * Gets the on_click_middle value
 *
 * @param self
 * @return The on_click_middle value
 */
gchar *wwt_config_get_on_click_middle(WwtConfig *self) {
    return self->on_click_middle;
}

/**
 * Gets the on_click_right value
 *
 * @param self
 * @return The on_click_right value
 */
gchar *wwt_config_get_on_click_right(WwtConfig *self) {
    return self->on_click_right;
}

/**
 * Gets the navigation_btn_on_click value
 *
 * @param self
 * @return The navigation_btn_on_click value
 */
gchar *wwt_config_get_navigation_btn_on_click(WwtConfig *self) {
    return self->navigation_btn_on_click;
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
    for(size_t i = 0; i < config_entries_len; i++) {
        const char *key = config_entries[i].key;
        const char *value = config_entries[i].value;

        g_autoptr(JsonParser) parser = create_json_parser(value);
        if(!parser) {
            continue;
        }

        JsonNode *node = json_parser_get_root(parser);

        if(strcmp("module_path", key) == 0) {
            continue;
        }

        if(strcmp("output", key) == 0) {
            const char *output = json_node_get_string(node);
            self->output = g_strdup(output);

            continue;
        }

        if(strcmp("window-manager", key) == 0) {
            const char *window_manager = json_node_get_string(node);

            if(strcmp("sway", window_manager) == 0) {
                self->window_manager_id = WM_ID_SWAY;
            } else if(strcmp("hyprland", window_manager) == 0) {
                self->window_manager_id = WM_ID_HYPRLAND;
            } else if(strcmp("niri", window_manager) == 0) {
                self->window_manager_id = WM_ID_NIRI;
            } else {
                self->window_manager_id = WM_ID_UNSUPPORTED;

                g_critical(
                    "Waybar Workspace Taskbar: unsupported window_manager %s",
                    window_manager
                );
            }

            continue;
        }

        if(strcmp("show-title", key) == 0) {
            int show_title = json_node_get_boolean(node);

            if(show_title) {
                self->show_title = TRUE;
            } else {
                self->show_title = FALSE;
            }

            continue;
        }

        if(strcmp("show-tooltip", key) == 0) {
            int show_tooltip = json_node_get_boolean(node);

            if(show_tooltip) {
                self->show_tooltip = TRUE;
            } else {
                self->show_tooltip = FALSE;
            }

            continue;
        }

        if(strcmp("show-icon", key) == 0) {
            int show_icon = json_node_get_boolean(node);

            if(show_icon) {
                self->show_title = TRUE;
            } else {
                self->show_icon = FALSE;
            }

            continue;
        }

        if(strcmp("max-tabs", config_entries[i].key) == 0) {
            int max_tabs = json_node_get_int(node);

            if(max_tabs > 0) {
                self->max_tabs = max_tabs;
            }

            continue;
        }

        if(strcmp("title-max-chars", config_entries[i].key) == 0) {
            int title_max_chars = json_node_get_int(node);

            if(title_max_chars > CONFIG_TITLE_MIN_CHARS) {
                self->title_max_chars = title_max_chars;
            }

            continue;
        }

        if(strcmp("text-align", config_entries[i].key) == 0) {
            const char *text_align = json_node_get_string(node);

            if(strcmp("left", text_align) == 0) {
                self->text_align = CONFIG_TAB_TEXT_ALIGN_LEFT;
            } else if(strcmp("right", text_align) == 0) {
                self->text_align = CONFIG_TAB_TEXT_ALIGN_RIGHT;
            } else {
                self->text_align = CONFIG_TAB_TEXT_ALIGN_CENTER;
            }

            continue;
        }

        if(strcmp("icon-size", config_entries[i].key) == 0) {
            int icon_size = json_node_get_int(node);

            if(icon_size > 0) {
                self->icon_size = icon_size;
            }

            continue;
        }

        if(strcmp("show-navigation-btns", key) == 0) {
            int show_navigation_btns = json_node_get_int(node);

            if(show_navigation_btns == NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW) {
                self->show_navigation_btns =
                    NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW;
            } else if(
                show_navigation_btns == NAVIGATION_BTN_DISPLAY_TYPE_ALWAYS
            ) {
                self->show_navigation_btns = NAVIGATION_BTN_DISPLAY_TYPE_ALWAYS;
            } else {
                self->show_navigation_btns = NAVIGATION_BTN_DISPLAY_TYPE_NEVER;
            }

            continue;
        }

        if(strcmp("navigation-btn-pos", config_entries[i].key) == 0) {
            const char navigation_btn_pos = json_node_get_int(node);

            if(navigation_btn_pos == NAVIGATION_BTN_POS_BEFORE) {
                self->navigation_btn_pos = NAVIGATION_BTN_POS_BEFORE;
            } else if(navigation_btn_pos == NAVIGATION_BTN_POS_AFTER) {
                self->navigation_btn_pos = NAVIGATION_BTN_POS_AFTER;
            } else {
                self->navigation_btn_pos = NAVIGATION_BTN_POS_STAGGERED;
            }

            continue;
        }

        if(strcmp("navigation-btn-prev-label", config_entries[i].key) == 0) {
            const char *navigation_btn_prev_label = json_node_get_string(node);

            g_free(self->navigation_btn_prev_label);
            self->navigation_btn_prev_label =
                g_strdup(navigation_btn_prev_label);

            continue;
        }

        if(strcmp("navigation-btn-next-label", config_entries[i].key) == 0) {
            const char *navigation_btn_next_label = json_node_get_string(node);

            g_free(self->navigation_btn_next_label);
            self->navigation_btn_next_label =
                g_strdup(navigation_btn_next_label);

            continue;
        }

        if(strcmp("on-click", key) == 0) {
            const char *on_click = json_node_get_string(node);
            self->on_click = g_strdup(on_click);

            continue;
        }

        if(strcmp("on-click-middle", key) == 0) {
            const char *on_click_middle = json_node_get_string(node);
            self->on_click_middle = g_strdup(on_click_middle);

            continue;
        }

        if(strcmp("on-click-right", key) == 0) {
            const char *on_click_right = json_node_get_string(node);
            self->on_click_right = g_strdup(on_click_right);

            continue;
        }

        if(strcmp("navigation-btn-on-click", key) == 0) {
            const char *navigation_btn_on_click = json_node_get_string(node);
            self->navigation_btn_on_click = g_strdup(navigation_btn_on_click);

            continue;
        }

        g_warning("Waybar Workspace Taskbar: Unknown config option %s", key);
    }
}

/**
 * Handles obj disposal
 *
 * @param obj The obj struct
 */
static void dispose(GObject *obj) {
    WwtConfig *self = WWT_CONFIG(obj);

    g_clear_pointer(&self->output, g_free);
    g_clear_pointer(&self->navigation_btn_prev_label, g_free);
    g_clear_pointer(&self->navigation_btn_next_label, g_free);
    g_clear_pointer(&self->on_click, g_free);
    g_clear_pointer(&self->on_click_middle, g_free);
    g_clear_pointer(&self->on_click_right, g_free);
    g_clear_pointer(&self->navigation_btn_on_click, g_free);

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
    self->show_tooltip = FALSE;
    self->title_max_chars = -1;
    self->max_tabs = -1;
    self->text_align = CONFIG_TAB_TEXT_ALIGN_CENTER;
    self->icon_size = 16;
    self->show_navigation_btns = NAVIGATION_BTN_DISPLAY_TYPE_NEVER;
    self->navigation_btn_pos = NAVIGATION_BTN_POS_STAGGERED;
    self->navigation_btn_prev_label = g_strdup("<");
    self->navigation_btn_next_label = g_strdup(">");
    self->on_click = NULL;
    self->on_click_middle = NULL;
    self->on_click_right = NULL;
    self->navigation_btn_on_click = NULL;
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
