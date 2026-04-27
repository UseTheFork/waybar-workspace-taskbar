#include "overflow_btn.h"
#include "core/app.h"
#include "core/config.h"

struct _WwtOverflowBtn {
    GtkButton parent_instance;

    WwtApp *app;
    WwtTaskbar *taskbar;
    OverflowBtnType type;
};

G_DEFINE_TYPE(WwtOverflowBtn, wwt_overflow_btn, GTK_TYPE_BUTTON);

#define OVERFLOW_BTN_START_CLASS_NAME "overflow-btn-start"
#define OVERFLOW_BTN_END_CLASS_NAME "overflow-btn-end"

/**
 * Handles the click action
 */
static void handle_click(gpointer user_data) {
    WwtOverflowBtn *self = user_data;

    if(self->type == OVERFLOW_BTN_START) {
        wwt_taskbar_shift_focus(self->taskbar, TASKBAR_FOCUS_PREV);
    } else {
        wwt_taskbar_shift_focus(self->taskbar, TASKBAR_FOCUS_NEXT);
    }
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_overflow_btn_init(WwtOverflowBtn *self) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
    if(self->type == OVERFLOW_BTN_START) {
        gtk_style_context_add_class(ctx, OVERFLOW_BTN_START_CLASS_NAME);
    } else {
        gtk_style_context_add_class(ctx, OVERFLOW_BTN_END_CLASS_NAME);
    }

    g_signal_connect(self, "clicked", G_CALLBACK(handle_click), self);
}

/**
 * Dispose the instance.
 *
 * @param obj The stuct obj
 */
static void dispose(GObject *obj) {
    WwtOverflowBtn *self = WWT_OVERFLOW_BTN(obj);

    G_OBJECT_CLASS(wwt_overflow_btn_parent_class)->dispose(obj);
}

/**
 * Finalizer for the instance
 *
 * @param object The struct obj
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_overflow_btn_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_overflow_btn_class_init(WwtOverflowBtnClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
    object_class->finalize = finalize;
}

/**
 * Creates a new overflow button
 *
 * @param app The app instance
 * @return The fully created overflow button
 */
WwtOverflowBtn *wwt_overflow_btn_new(
    WwtApp *app,
    WwtTaskbar *taskbar,
    OverflowBtnType type
) {
    WwtConfig *config = wwt_app_get_config(app);
    const gchar *overflow_btn_start_label =
        wwt_config_get_overflow_btn_start_label(config);
    const gchar *overflow_btn_end_label =
        wwt_config_get_overflow_btn_end_label(config);

    const char *label = type == OVERFLOW_BTN_START ? overflow_btn_start_label
                                                   : overflow_btn_end_label;

    WwtOverflowBtn *self =
        g_object_new(WWT_OVERFLOW_BUTTON_TYPE, "label", label, NULL);
    self->app = app;
    self->taskbar = taskbar;
    self->type = type;

    return self;
}
