#pragma once

#include "wbcffi.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _WwtTaskbar WwtTaskbar;
typedef struct _WwtConfig WwtConfig;
typedef struct _WwtServices WwtServices;

#define WWT_APP_TYPE (wwt_app_get_type())

G_DECLARE_FINAL_TYPE(WwtApp, wwt_app, WWT, APP, GObject)

WwtApp *wwt_app_new(
    const wbcffi_init_info *init_info,
    const wbcffi_config_entry *config_entries,
    size_t config_entries_len
);

WwtTaskbar *wwt_app_get_taskbar(WwtApp *self);
WwtConfig *wwt_app_get_config(WwtApp *self);
WwtServices *wwt_app_get_services(WwtApp *self);

G_END_DECLS;
