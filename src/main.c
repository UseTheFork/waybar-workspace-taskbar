#include "core/app.h"
#include "wbcffi.h"

// You must
const size_t wbcffi_version = 2;

void *wbcffi_init(
    const wbcffi_init_info *init_info,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
) {
    return wwt_app_new(init_info, config_entries, config_entries_len);
}

void wbcffi_deinit(void *instance) {
    if (instance) {
        g_object_unref(instance);
    }
}
