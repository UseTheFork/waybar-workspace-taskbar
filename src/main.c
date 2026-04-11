#include "core/app.h"
#include "wbcffi.h"

// You must
const size_t wbcffi_version = 2;

/**
 * Waybar init
 *
 * @param init_info Waybar module information
 * @param config_entries Module configuration entries
 * @return Instance of the app or NULL on failure
 */
void *wbcffi_init(
    const wbcffi_init_info *init_info,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    return wwt_app_new(init_info, config_entries, config_entries_len);
}

/**
 * Waybar deinit called when waybar closes
 *
 * @param instance The app instance
 */
void wbcffi_deinit(void *instance) {
    if(instance) {
        g_object_unref(instance);
    }
}
