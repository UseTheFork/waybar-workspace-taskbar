#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct WindowManagerSpec WindowManagerSpec;
typedef struct _WindowManagerEvents WindowManagerEvents;
typedef struct _AppIcons AppIcons;
typedef struct _WwtConfig WwtConfig;

#define WWT_SERVICES_TYPE (wwt_services_get_type())

G_DECLARE_FINAL_TYPE(WwtServices, wwt_services, WWT, SERVICES, GObject)

WwtServices *wwt_services_default(WwtConfig *config);
WindowManagerEvents *wwt_services_get_window_manager_events(WwtServices *self);
WindowManagerSpec *wwt_services_get_window_manager_spec(WwtServices *self);
AppIcons *wwt_services_get_app_icons(WwtServices *self);

G_END_DECLS;
