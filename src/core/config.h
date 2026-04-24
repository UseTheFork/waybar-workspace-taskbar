#pragma once

#include "wbcffi.h"
#include "window_manager.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define TAB_TEXT_ALIGN_LEFT 0.0
#define TAB_TEXT_ALIGN_CENTER 0.5
#define TAB_TEXT_ALIGN_RIGHT 1.0

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

WwtConfig *wwt_config_new(
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
);

G_END_DECLS;
