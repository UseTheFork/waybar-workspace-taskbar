#pragma once

#include "core/app.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_TABS_TYPE (wwt_tabs_get_type())

G_DECLARE_FINAL_TYPE(WwtTabs, wwt_tabs, WWT, TABS, GtkBox)

WwtTabs *wwt_tabs_new(WwtApp *app);
gboolean wwt_tabs_generate_tabs(WwtTabs *self);

G_END_DECLS;
