#include "taskbar.h"
#include "core/app.h"
#include "core/config.h"
#include "core/services.h"
#include "navigation_btn.h"
#include "services/window_manager/data.h"
#include "services/window_manager/subscriptions.h"
#include "tab.h"

struct _WwtTaskbar {
    GtkBox parent_instance;

    WwtApp *app;
    WwtNavigationBtn *navigation_btn_prev;
    WwtNavigationBtn *navigation_btn_next;
    GtkBox *tabs;
    int events_subscription_id;
    int focused_idx;
};

G_DEFINE_TYPE(WwtTaskbar, wwt_taskbar, GTK_TYPE_BOX);

#define TABS_CLASS_NAME "tabs"
#define TASKBAR_CLASS_NAME "taskbar"
#define TASKBAR_CLASS_NAME_EMPTY "empty"
#define TASKBAR_CLASS_NAME_SINGLE "single"
#define TASKBAR_CLASS_NAME_OVERFLOW_START "overflow-start"
#define TASKBAR_CLASS_NAME_OVERFLOW_END "overflow-end"

/**
 * Gets the focused idx value
 *
 * @param self
 */
int wwt_taskbar_get_focused_idx(WwtTaskbar *self) {
    return self->focused_idx;
}

/**
 * Handles the visual state after tab generation.
 *
 * @param tabs The tabs instance
 * @param tab_len How many tabs are in the bar
 * @param overflow_start Whether there is max tabs overflow at the start
 * @param overflow_end Whether there is max tabs overflow at the end
 */
static void apply_visual_styles(
    WwtTaskbar *self,
    int tab_len,
    gboolean overflow_start,
    gboolean overflow_end
) {
    WwtConfig *config = wwt_app_get_config(self->app);
    gboolean show_navigation_btns = wwt_config_get_show_navigation_btns(config);

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

    if(show_navigation_btns &&
        show_navigation_btns == NAVIGATION_BTN_DISPLAY_TYPE_OVERFLOW) {
        if(overflow_start || overflow_end) {
            gtk_widget_show(GTK_WIDGET(self->navigation_btn_prev));
            gtk_widget_show(GTK_WIDGET(self->navigation_btn_next));
        } else {
            gtk_widget_hide(GTK_WIDGET(self->navigation_btn_prev));
            gtk_widget_hide(GTK_WIDGET(self->navigation_btn_next));
        }
    }
}

/**
 * Gets data from the window manager and renders the tabs
 *
 * @param self
 */
static void wwt_taskbar_update_tabs(
    WwtTaskbar *self,
    WwtWindowManagerData *wm_data
) {
    WwtConfig *config = wwt_app_get_config(self->app);
    const char *output = wwt_config_get_output(config);

    int max_tabs = wwt_config_get_max_tabs(config);
    GPtrArray *wins = wwt_window_manager_data_get_windows(wm_data, output);

    if(!wins) {
        return;
    }

    int focused_idx = wwt_window_manager_data_get_focused_idx(wm_data, output);
    self->focused_idx = focused_idx > -1 ? focused_idx : self->focused_idx;

    int start = 0;
    int end = wins->len;

    if(max_tabs > 0 && (int)wins->len > max_tabs) {
        start = self->focused_idx - max_tabs / 2;
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

    GtkBox *tabs = self->tabs;
    GList *children = gtk_container_get_children(GTK_CONTAINER(tabs));
    GList *child = children;

    for(guint i = start; i < end; i++) {
        WindowManagerDataWindow *win = g_ptr_array_index(wins, i);

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

            gtk_container_add(GTK_CONTAINER(tabs), GTK_WIDGET(tab));
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
        gtk_container_remove(GTK_CONTAINER(tabs), GTK_WIDGET(child->data));
        child = next;
    }

    g_list_free(children);
    gtk_widget_show_all(GTK_WIDGET(self));

    gboolean overflow_start = start > 0;
    gboolean overflow_end = end < (int)wins->len;
    apply_visual_styles(self, wins->len, overflow_start, overflow_end);
}

/**
 * Event listener for the window manager
 *
 * @param data The notify data (Window Manager Data)
 * @param user_data The data passed in (self)
 */
static void event_listener(gpointer data, gpointer user_data) {
    WwtWindowManagerData *wm_data = data;
    WwtTaskbar *self = user_data;

    wwt_taskbar_update_tabs(self, wm_data);
}

/**
 * Sets up the widgets used by the taskbar
 *
 * @param self
 */
static void setup_widgets(WwtTaskbar *self) {
    WwtConfig *config = wwt_app_get_config(self->app);
    gboolean show_navigation_btns = wwt_config_get_show_navigation_btns(config);
    NavigationBtnPos navigation_btn_pos =
        wwt_config_get_navigation_btn_pos(config);

    if(show_navigation_btns) {
        self->navigation_btn_prev =
            wwt_navigation_btn_new(self->app, self, NAVIGATION_BTN_TYPE_PREV);
        self->navigation_btn_next =
            wwt_navigation_btn_new(self->app, self, NAVIGATION_BTN_TYPE_NEXT);
    }

    self->tabs = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

    if(show_navigation_btns && navigation_btn_pos != NAVIGATION_BTN_POS_AFTER) {
        gtk_box_pack_start(
            GTK_BOX(self),
            GTK_WIDGET(self->navigation_btn_prev),
            FALSE,
            FALSE,
            0
        );
        if(navigation_btn_pos == NAVIGATION_BTN_POS_BEFORE) {
            gtk_box_pack_start(
                GTK_BOX(self),
                GTK_WIDGET(self->navigation_btn_next),
                FALSE,
                FALSE,
                0
            );
        }
    }

    gtk_box_pack_start(GTK_BOX(self), GTK_WIDGET(self->tabs), TRUE, TRUE, 0);

    if(show_navigation_btns &&
        navigation_btn_pos != NAVIGATION_BTN_POS_BEFORE) {
        if(navigation_btn_pos == NAVIGATION_BTN_POS_AFTER) {
            gtk_box_pack_start(
                GTK_BOX(self),
                GTK_WIDGET(self->navigation_btn_prev),
                FALSE,
                FALSE,
                0
            );
        }
        gtk_box_pack_start(
            GTK_BOX(self),
            GTK_WIDGET(self->navigation_btn_next),
            FALSE,
            FALSE,
            0
        );
    }

    // Set styles
    GtkStyleContext *taskbar_style_ctx =
        gtk_widget_get_style_context(GTK_WIDGET(self));
    gtk_style_context_add_class(taskbar_style_ctx, TASKBAR_CLASS_NAME);

    GtkStyleContext *tabs_style_ctx =
        gtk_widget_get_style_context(GTK_WIDGET(self->tabs));
    gtk_style_context_add_class(tabs_style_ctx, TABS_CLASS_NAME);
}

/**
 * Initialize the instance
 *
 * @param self
 */
static void wwt_taskbar_init(WwtTaskbar *self) {
}

/**
 * Dispose the instance
 *
 * @param obj The stuct obj
 */
static void dispose(GObject *obj) {
    WwtTaskbar *self = WWT_TASKBAR(obj);

    if(self->events_subscription_id >= 0) {
        WwtServices *services = wwt_app_get_services(self->app);
        WwtWindowManager *wm = wwt_services_get_window_manager(services);
        WwtWindowManagerSubscriptions *subscriptions =
            wwt_window_manager_get_subsciptions(wm);

        wwt_window_manager_subscriptions_unsubscribe(
            subscriptions,
            self->events_subscription_id
        );
        self->events_subscription_id = -1;
    }

    G_OBJECT_CLASS(wwt_taskbar_parent_class)->dispose(obj);
}

/**
 * Finalizer for the instance
 *
 * @param object The struct obj
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
 * Creates a new instance of a taskbar
 *
 * @param app The app instance
 * @return The fully created taskbar widget
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

    self->app = app;
    self->focused_idx = 0;
    setup_widgets(self);

    WwtServices *services = wwt_app_get_services(app);
    if(!services) {
        g_object_unref(self);
        return NULL;
    }

    WwtWindowManager *wm = wwt_services_get_window_manager(services);

    WwtWindowManagerSubscriptions *subscriptions =
        wwt_window_manager_get_subsciptions(wm);
    self->events_subscription_id = wwt_window_manager_subscriptions_subscribe(
        subscriptions,
        event_listener,
        self
    );

    WwtWindowManagerData *wm_data = wwt_window_manager_get_data(wm);

    wwt_taskbar_update_tabs(self, wm_data);

    return self;
}
