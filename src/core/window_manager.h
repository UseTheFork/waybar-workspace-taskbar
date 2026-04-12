#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_WINDOW_MANAGER_TYPE (wwt_window_manager_get_type())

G_DECLARE_FINAL_TYPE(
    WwtWindowManager,
    wwt_window_manager,
    WWT,
    WINDOW_MANAGER,
    GObject
)

typedef struct _WwtApp WwtApp;
typedef struct _WwtWindowManager WwtWindowManager;
typedef struct WindowManagerSpec WindowManagerSpec;
typedef struct _WindowManagerEvents WindowManagerEvents;

typedef enum {
    WM_ID_UNSUPPORTED,
    WM_ID_SWAY,
    WM_ID_NIRI,
    WM_ID_HYPRLAND
} WindowManagerId;

WwtWindowManager *wwt_window_manager_default(WindowManagerId id);
WwtWindowManager *wwt_window_manager_instance();
WindowManagerSpec *wwt_window_manager_get_spec(WwtWindowManager *self);
WindowManagerEvents *wwt_window_manager_get_events(WwtWindowManager *self);

G_END_DECLS;
