#include "wbcffi.h"
#include "window_manager.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_CONFIG_TYPE (wwt_config_get_type())

G_DECLARE_FINAL_TYPE(WwtConfig, wwt_config, WWT, CONFIG, GObject)

WindowManagerId wwt_config_get_wm_id(WwtConfig *self);

WwtConfig *wwt_config_new(
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
);

G_END_DECLS;
