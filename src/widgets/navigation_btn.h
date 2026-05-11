#pragma once

#include "taskbar.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WWT_NAVIGATION_BUTTON_TYPE (wwt_navigation_btn_get_type())
G_DECLARE_FINAL_TYPE(
    WwtNavigationBtn,
    wwt_navigation_btn,
    WWT,
    NAVIGATION_BTN,
    GtkButton
)

typedef enum NavigationBtnDisplayType {
    NAVIGATION_BTN_DISPLAY_TYPE_NEVER = 0,
    NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW = 1,
    NAVIGATION_BTN_DISPLAY_TYPE_ALWAYS = 2
} NavigationBtnDisplayType;

typedef enum NavigationBtnType {
    NAVIGATION_BTN_TYPE_START,
    NAVIGATION_BTN_TYPE_END
} NavigationBtnType;

typedef enum NavigationBtnPos {
    NAVIGATION_BTN_POS_STAGGERED = 0,
    NAVIGATION_BTN_POS_BEFORE = 1,
    NAVIGATION_BTN_POS_AFTER = 2
} NavigationBtnPos;

WwtNavigationBtn *wwt_navigation_btn_new(
    WwtApp *app,
    WwtTaskbar *taskbar,
    NavigationBtnType type
);

G_END_DECLS
