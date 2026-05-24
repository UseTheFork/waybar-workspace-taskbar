#include "navigation_btn.h"
#include "core/app.h"
#include "core/config.h"
#include "utils/cmd.h"
#include "utils/common.h"

struct _WwtNavigationBtn {
    GtkButton parent_instance;

    WwtApp *app;
    WwtTaskbar *taskbar;
    NavigationBtnType type;
};

G_DEFINE_TYPE(WwtNavigationBtn, wwt_navigation_btn, GTK_TYPE_BUTTON);

#define NAVIGATION_BTN_PREV_CLASS_NAME "navigation-btn-prev"
#define NAVIGATION_BTN_NEXT_CLASS_NAME "navigation-btn-next"

/**
 * Handles the click action
 *
 * @param user_data User defined data (self in this instance)
 */
static void handle_click(gpointer user_data) {
    WwtNavigationBtn *self = user_data;
    WwtConfig *config = wwt_app_get_config(self->app);
    const char *navigation_btn_on_click =
        wwt_config_get_navigation_btn_on_click(config);

    if(self->type == NAVIGATION_BTN_TYPE_PREV) {
        gchar *win_id = wwt_taskbar_get_focus_win_id(self->taskbar, -1);

        if(win_id) {
            g_autofree gchar *cmd =
                str_replace(navigation_btn_on_click, "{id}", win_id);
            cmd_run_fork_exec(cmd);
        }

    } else {
        gchar *win_id = wwt_taskbar_get_focus_win_id(self->taskbar, 1);

        if(win_id) {
            g_autofree gchar *cmd =
                str_replace(navigation_btn_on_click, "{id}", win_id);
            cmd_run_fork_exec(cmd);
        }
    }
}

/**
 * Sets up the widget properties and styles
 *
 * @param self
 */
static void setup_widget(WwtNavigationBtn *self) {
    WwtConfig *config = wwt_app_get_config(self->app);
    NavigationBtnDisplayType show_navigation_btns =
        wwt_config_get_show_navigation_btns(config);

    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
    if(self->type == NAVIGATION_BTN_TYPE_PREV) {
        gtk_style_context_add_class(ctx, NAVIGATION_BTN_PREV_CLASS_NAME);
    } else {
        gtk_style_context_add_class(ctx, NAVIGATION_BTN_NEXT_CLASS_NAME);
    }

    if(show_navigation_btns == NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW) {
        gtk_widget_set_no_show_all(GTK_WIDGET(self), TRUE);
        gtk_widget_hide(GTK_WIDGET(self));
    }

    if(show_navigation_btns &&
        show_navigation_btns == NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW) {
        gtk_widget_hide(GTK_WIDGET(self));
    }
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_navigation_btn_init(WwtNavigationBtn *self) {
    g_signal_connect(self, "clicked", G_CALLBACK(handle_click), self);
}

/**
 * Dispose the instance.
 *
 * @param obj The stuct obj
 */
static void dispose(GObject *obj) {
    WwtNavigationBtn *self = WWT_NAVIGATION_BTN(obj);

    G_OBJECT_CLASS(wwt_navigation_btn_parent_class)->dispose(obj);
}

/**
 * Finalizer for the instance
 *
 * @param object The struct obj
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_navigation_btn_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_navigation_btn_class_init(WwtNavigationBtnClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
    object_class->finalize = finalize;
}

/**
 * Creates a new navigation button
 *
 * @param app The app instance
 * @return The fully created navigation button
 */
WwtNavigationBtn *wwt_navigation_btn_new(
    WwtApp *app,
    WwtTaskbar *taskbar,
    NavigationBtnType type
) {
    WwtConfig *config = wwt_app_get_config(app);

    const gchar *navigation_btn_prev_label =
        wwt_config_get_navigation_btn_prev_label(config);
    const gchar *navigation_btn_next_label =
        wwt_config_get_navigation_btn_next_label(config);

    const char *label = type == NAVIGATION_BTN_TYPE_PREV
                            ? navigation_btn_prev_label
                            : navigation_btn_next_label;

    WwtNavigationBtn *self =
        g_object_new(WWT_NAVIGATION_BUTTON_TYPE, "label", label, NULL);

    self->app = app;
    self->taskbar = taskbar;
    self->type = type;

    setup_widget(self);

    return self;
}
