#pragma once

#include "taskbar.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define WWT_OVERFLOW_BUTTON_TYPE (wwt_overflow_btn_get_type())
G_DECLARE_FINAL_TYPE(
    WwtOverflowBtn,
    wwt_overflow_btn,
    WWT,
    OVERFLOW_BTN,
    GtkButton
)

typedef enum OverflowBtnType {
    OVERFLOW_BTN_START,
    OVERFLOW_BTN_END
} OverflowBtnType;

WwtOverflowBtn *wwt_overflow_btn_new(
    WwtApp *app,
    WwtTaskbar *taskbar,
    OverflowBtnType type
);

G_END_DECLS;
