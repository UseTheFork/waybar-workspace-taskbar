#include "taskbar.h"
#include "core/app.h"
#include "core/config.h"
#include "core/window_manager.h"
#include "core/window_manager_data.h"
#include "core/window_manager_events.h"
#include "tab.h"

struct _WwtTaskbar {
    GtkBox parent_instance;

    WwtApp *app;
    int events_subscription_id;
};

G_DEFINE_TYPE(WwtTaskbar, wwt_taskbar, GTK_TYPE_BOX);

#define TASKBAR_CLASS_NAME "taskbar"
#define TASKBAR_CLASS_NAME_EMPTY "empty"
#define TASKBAR_CLASS_NAME_SINGLE "single"
#define TASKBAR_CLASS_NAME_OVERFLOW_START "overflow-start"
#define TASKBAR_CLASS_NAME_OVERFLOW_END "overflow-end"

/**
 * Handles the visual state after tab generation.
 *
 * @param tabs The tabs instance
 * @param tab_len How many tabs are in the bar
 * @param overflow_start Whether there is max tabs overflow at the start
 * @param overflow_end Whether there is max tabs overflow at the end
 */
void wwt_taskbar_apply_class_names(
    WwtTaskbar *self,
    int tab_len,
    gboolean overflow_start,
    gboolean overflow_end
) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));

    if(tab_len == 0) {
        gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME_EMPTY);

    } else {
        gtk_style_context_remove_class(ctx, TASKBAR_CLASS_NAME_EMPTY);
    }

    if(tab_len == 1) {
        gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME_SINGLE);
    } else {
        gtk_style_context_remove_class(ctx, TASKBAR_CLASS_NAME_SINGLE);
    }

    if(overflow_start) {
        gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME_OVERFLOW_START);
    } else {
        gtk_style_context_remove_class(ctx, TASKBAR_CLASS_NAME_OVERFLOW_START);
    }

    if(overflow_end) {
        gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME_OVERFLOW_END);
    } else {
        gtk_style_context_remove_class(ctx, TASKBAR_CLASS_NAME_OVERFLOW_END);
    }
}

/**
 * Get the index of the focused window
 *
 * @param wins The array of windows
 * @return The focused windows index
 */
static int get_focused_index(GPtrArray *wins) {
    for(guint i = 0; i < wins->len; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);
        if(win->focused) {
            return i;
        }
    }

    return 0;
}

/**
 * Generates the tabs
 *
 * @param bar The tabs instance
 */
void wwt_taskbar_populate_tabs(WindowManagerData *wm_data, gpointer user_data) {
    WwtTaskbar *self = user_data;
    WwtConfig *config = wwt_app_get_config(self->app);

    int max_tabs = wwt_config_get_max_tabs(config);
    const gchar *output = wwt_config_get_output(config);

    GPtrArray *wins;

    if(output) {
        wins = window_manager_data_get_windows_on_output(wm_data, output);
    } else {
        wins = window_manager_data_get_windows_on_focused(wm_data);
    }

    if(!wins) {
        return;
    }

    int focused_index = get_focused_index(wins);
    int start = 0;
    int end = wins->len;

    if(max_tabs > 0 && (int)wins->len > max_tabs) {
        start = focused_index - max_tabs / 2;
        end = start + max_tabs;

        if(start < 0) {
            start = 0;
            end = max_tabs;
        }

        if(end > (int)wins->len) {
            end = wins->len;
            start = end - max_tabs;
        }
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(self));
    GList *child = children;

    for(guint i = start; i < end; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);

        if(child == NULL) {
            WwtTab *tab = wwt_tab_new(
                self->app,
                win->id,
                win->title,
                win->app_id,
                win->focused,
                win->floating,
                win->urgent,
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
                win->floating,
                win->urgent,
                win->x,
                win->y
            );
        }

        if(child) {
            child = child->next;
        }
    }

    while(child != NULL) {
        GList *next = child->next;
        gtk_container_remove(GTK_CONTAINER(self), GTK_WIDGET(child->data));
        child = next;
    }

    g_list_free(children);
    gtk_widget_show_all(GTK_WIDGET(self));

    gboolean overflow_start = start > 0;
    gboolean overflow_end = end < (int)wins->len;
    wwt_taskbar_apply_class_names(
        self,
        wins->len,
        overflow_start,
        overflow_end
    );

    return;
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
 */
static void dispose(GObject *obj) {
    WwtTaskbar *self = WWT_TASKBAR(obj);
    WwtWindowManager *wm = wwt_window_manager_instance();

    if(wm) {
        WindowManagerEvents *events = wwt_window_manager_get_events(wm);
        window_manager_events_unsubscribe(events, self->events_subscription_id);
        self->events_subscription_id = -1;
    }

    G_OBJECT_CLASS(wwt_taskbar_parent_class)->dispose(obj);
}

/**
 * Finalizer for the tabs instance
 *
 * @param object The tabs struct
 */
static void finalize(GObject *obj) {
    G_OBJECT_CLASS(wwt_taskbar_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_taskbar_class_init(WwtTaskbarClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
    object_class->finalize = finalize;
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

    WwtWindowManager *wm = wwt_window_manager_instance();

    if(!wm) {
        g_object_unref(self);
        return NULL;
    }

    WindowManagerEvents *wm_events = wwt_window_manager_get_events(wm);
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));

    gtk_style_context_add_class(ctx, TASKBAR_CLASS_NAME);

    self->app = app;

    self->events_subscription_id = window_manager_events_subscribe(
        wm_events,
        wwt_taskbar_populate_tabs,
        self
    );

    return self;
}
