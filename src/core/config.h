#pragma once

#include "wbcffi.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum WindowManagerId WindowManagerId;
typedef enum NavigationBtnPos NavigationBtnPos;

#define CONFIG_TAB_TEXT_ALIGN_LEFT 0.0
#define CONFIG_TAB_TEXT_ALIGN_CENTER 0.5
#define CONFIG_TAB_TEXT_ALIGN_RIGHT 1.0
#define CONFIG_TITLE_MIN_CHARS 3

#define WWT_CONFIG_TYPE (wwt_config_get_type())

G_DECLARE_FINAL_TYPE(WwtConfig, wwt_config, WWT, CONFIG, GObject)

WindowManagerId wwt_config_get_wm_id(WwtConfig *self);
const char *wwt_config_get_output(WwtConfig *self);
gboolean wwt_config_get_show_icon(WwtConfig *self);
gboolean wwt_config_get_show_title(WwtConfig *self);
gboolean wwt_config_get_show_tooltip(WwtConfig *self);
int wwt_config_get_max_tabs(WwtConfig *self);
int wwt_config_get_title_max_chars(WwtConfig *self);
int wwt_config_get_title_max_chars(WwtConfig *self);
gfloat wwt_config_get_text_align(WwtConfig *self);
int wwt_config_get_icon_size(WwtConfig *self);
gboolean wwt_config_get_show_navigation_btns(WwtConfig *self);
gchar *wwt_config_get_navigation_btn_prev_label(WwtConfig *self);
gchar *wwt_config_get_navigation_btn_next_label(WwtConfig *self);
NavigationBtnPos wwt_config_get_navigation_btn_pos(WwtConfig *self);
gchar *wwt_config_get_on_click(WwtConfig *self);
gchar *wwt_config_get_on_click_middle(WwtConfig *self);
gchar *wwt_config_get_on_click_right(WwtConfig *self);
gchar *wwt_config_get_navigation_btn_on_click(WwtConfig *self);

WwtConfig *wwt_config_new(
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
);

G_END_DECLS
