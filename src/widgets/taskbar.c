#include "taskbar.h"
#include "core/app.h"
#include "core/config.h"
#include "core/window_manager.h"
#include "tab.h"
#include "window_managers/common.h"

struct _WwtTaskbar {
    GtkBox parent_instance;

    WwtApp *app;
};

G_DEFINE_TYPE(WwtTaskbar, wwt_taskbar, GTK_TYPE_BOX);

#define TASKBAR_CLASS_NAME "taskbar"

/**
 * Handles the visual state after tab generation.
 *
 * @param tabs The tabs instance
 * @param tab_len How many tabs are in the bar
 * @param overflow_start Whether there is max tabs overflow at the start
 * @param overflow_end Whether there is max tabs overflow at the end
 */
void wwt_taskbar_apply_visual_state(
    WwtTaskbar *tabs,
    int tab_len,
    gboolean overflow_start,
    gboolean overflow_end
) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(tabs));

    if (tab_len == 0) {
        gtk_style_context_add_class(ctx, "empty-tabs");

    } else {
        gtk_style_context_remove_class(ctx, "empty-tabs");
    }

    if (tab_len == 1) {
        gtk_style_context_add_class(ctx, "single-tab");
    } else {
        gtk_style_context_remove_class(ctx, "single-tab");
    }

    if (overflow_start) {
        gtk_style_context_add_class(ctx, "overflow-start");
    } else {
        gtk_style_context_remove_class(ctx, "overflow-start");
    }

    if (overflow_end) {
        gtk_style_context_add_class(ctx, "overflow-end");
    } else {
        gtk_style_context_remove_class(ctx, "overflow-end");
    }
}

/**
 * Get the index of the focused window
 *
 * @param wins The array of windows
 * @return The focused windows index
 */
static int get_focused_index(GPtrArray *wins) {
    for (guint i = 0; i < wins->len; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);
        if (win->focused) {
            return i;
        }
    }

    return 0;
}

/**
 * Generates the tabs
 *
 * @param bar The tabs instance
 * @return TRUE if tabs were successfully genenerated else FALSE
 */
gboolean wwt_taskbar_generate_tabs(WwtTaskbar *self) {
    WwtWindowManager *wm = wwt_app_get_window_manager(self->app);
    WwtConfig *config = wwt_app_get_config(self->app);
    int max_tabs = wwt_config_get_max_tabs(config);

    WindowManagerGetWindows get_windows =
        wwt_window_manager_get_get_windows(wm);

    GPtrArray *wins =
        g_ptr_array_new_with_free_func((GDestroyNotify)wm_win_destroy);

    gboolean fetched = get_windows(self->app, wins);

    if (!fetched) {
        g_ptr_array_free(wins, TRUE);
        return FALSE;
    }

    int focused_index = get_focused_index(wins);
    int start = 0;
    int end = wins->len;

    if (max_tabs > 0 && (int)wins->len > max_tabs) {
        start = focused_index - max_tabs / 2;
        end = start + max_tabs;

        if (start < 0) {
            start = 0;
            end = max_tabs;
        }

        if (end > (int)wins->len) {
            end = wins->len;
            start = end - max_tabs;
        }
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(self));
    GList *child = children;

    for (guint i = start; i < end; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);

        if (child == NULL) {
            WwtTab *tab = wwt_tab_new(
                self->app,
                win->id,
                win->title,
                win->app_id,
                win->focused,
                win->x,
                win->y
            );

            gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(tab));
        } else {
            wwt_tab_update(
                WWT_TAB(child->data),
                win->id,
                win->title,
                win->app_id,
                win->focused,
                win->x,
                win->y
            );
        }

        if (child) {
            child = child->next;
        }
    }

    while (child != NULL) {
        GList *next = child->next;
        gtk_container_remove(GTK_CONTAINER(self), GTK_WIDGET(child->data));
        child = next;
    }

    g_list_free(children);
    gtk_widget_show_all(GTK_WIDGET(self));

    gboolean overflow_start = start > 0;
    gboolean overflow_end = end < (int)wins->len;
    wwt_taskbar_apply_visual_state(
        self,
        wins->len,
        overflow_start,
        overflow_end
    );

    g_ptr_array_free(wins, TRUE);

    return TRUE;
}

/**
 * Initialize the tabs instance
 *
 * @param self
 */
static void wwt_taskbar_init(WwtTaskbar *self) {
}

/**
 * Dispose the tabs instance. Gref cleanup.
 *
 * @param obj The stuct obj.
 *
 */
static void wwt_taskbar_dispose(GObject *obj) {
    WwtTaskbar *self = WWT_TASKBAR(obj);

    printf("calling dispose on tabs\n");

    G_OBJECT_CLASS(wwt_taskbar_parent_class)->dispose(obj);
}

/**
 * Finalizer for the tabs instance
 *
 * @param object The tabs struct
 */
static void wwt_taskbar_finalize(GObject *obj) {
    WwtTaskbar *self = WWT_TASKBAR(obj);

    printf("calling finalize on tabs\n");

    G_OBJECT_CLASS(wwt_taskbar_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_taskbar_class_init(WwtTaskbarClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = wwt_taskbar_dispose;
    object_class->finalize = wwt_taskbar_finalize;
}

/**
 * Creates a new instance of the tabs widget
 *
 * @param app The app instance
 * @return The fully created tabs widget
 */
WwtTaskbar *wwt_taskbar_new(WwtApp *app) {
    WwtTaskbar *self = g_object_new(
        WWT_TASKBAR_TYPE,
        "orientation",
        GTK_ORIENTATION_HORIZONTAL,
        "spacing",
        0,
        NULL
    );

    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));

    gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME);

    self->app = app;

    return self;
}
