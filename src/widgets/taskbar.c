#include "taskbar.h"
#include "core/app.h"
#include "core/config.h"
#include "core/services.h"
#include "core/window_manager_data.h"
#include "glibconfig.h"
#include "navigation_btn.h"
#include "services/window_manager_events.h"
#include "services/window_manager_spec.h"
#include "tab.h"

struct _WwtTaskbar {
    GtkBox parent_instance;

    WwtApp *app;
    WwtNavigationBtn *navigation_btn_prev;
    WwtNavigationBtn *navigation_btn_next;
    GtkBox *tabs;
    WindowManagerData *wm_data;
    int events_subscription_id;
    int prev_focused_index;
};

G_DEFINE_TYPE(WwtTaskbar, wwt_taskbar, GTK_TYPE_BOX);

#define TABS_CLASS_NAME "tabs"
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
 * Get the index of the focused window
 *
 * @param wins The array of windows
 * @return The focused windows index
 */
static int get_focused_index(WwtTaskbar *self, GPtrArray *wins) {
    if(wins->len == 0) {
        self->prev_focused_index = 0;
        return 0;
    }

    for(guint i = 0; i < wins->len; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);

        if(win->focused) {
            self->prev_focused_index = i;
            return i;
        }
    }

    self->prev_focused_index =
        CLAMP(self->prev_focused_index, 0, (int)wins->len - 1);

    return self->prev_focused_index;
}

/**
 * Gets the windows to be displayed in tabs
 *
 * @param self
 * @return A GPtrArray of tabs to be displayed
 */
static GPtrArray *get_display_windows(WwtTaskbar *self) {
    WwtConfig *config = wwt_app_get_config(self->app);
    const gchar *output = wwt_config_get_output(config);

    GPtrArray *wins = NULL;
    if(output) {
        wins = window_manager_data_get_windows_on_output(self->wm_data, output);
    } else {
        wins = window_manager_data_get_windows_on_focused(self->wm_data);
    }

    return wins;
}

/**
 * Focus the next window in the list
 *
 * @param self
 */
void wwt_taskbar_shift_focus(
    WwtTaskbar *self,
    TaskbarFocusDirection direction
) {
    WwtServices *services = wwt_app_get_services(self->app);
    WindowManagerSpec *spec = wwt_services_get_window_manager_spec(services);
    WindowManagerClickHandler focus_window =
        window_manager_spec_get_click_handler(spec, WM_CLICK_FOCUS);
    GPtrArray *wins = get_display_windows(self);

    if(!wins || wins->len == 0) {
        return;
    }

    int focused_index = get_focused_index(self, wins);
    int index = 0;

    if(direction == TASKBAR_FOCUS_NEXT) {
        index = CLAMP(focused_index + 1, 0, (int)wins->len - 1);
    } else {
        index = CLAMP(focused_index - 1, 0, (int)wins->len - 1);
    }

    WindowManagerWindow *win = g_ptr_array_index(wins, index);
    focus_window(win->id);
}

/**
 * Gets data from the window manager and renders the tabs
 *
 * @param self
 */
void wwt_taskbar_update_tabs(WwtTaskbar *self) {
    WwtServices *services = wwt_app_get_services(self->app);
    WwtConfig *config = wwt_app_get_config(self->app);
    WindowManagerSpec *spec = wwt_services_get_window_manager_spec(services);

    WindowManagerDataFetcher fetch_data =
        window_manager_spec_get_data_fetcher(spec);

    window_manager_data_destroy(self->wm_data);
    self->wm_data = fetch_data();

    if(!self->wm_data) {
        return;
    }

    int max_tabs = wwt_config_get_max_tabs(config);
    GPtrArray *wins = get_display_windows(self);

    if(!wins) {
        return;
    }

    int focused_index = get_focused_index(self, wins);
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

    GtkBox *tabs = self->tabs;
    GList *children = gtk_container_get_children(GTK_CONTAINER(tabs));
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
 * @param event The window manager event
 * @param user_data The data passed in (self)
 */
static void event_listener(WindowManagerEvent *event, gpointer user_data) {
    WwtTaskbar *self = user_data;
    wwt_taskbar_update_tabs(self);
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
            wwt_navigation_btn_new(self->app, self, NAVIGATION_BTN_TYPE_START);
        self->navigation_btn_next =
            wwt_navigation_btn_new(self->app, self, NAVIGATION_BTN_TYPE_END);
    }

    self->tabs = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

    if(show_navigation_btns &&
        (navigation_btn_pos == NAVIGATION_BTN_POS_BEFORE ||
            navigation_btn_pos == NAVIGATION_BTN_POS_STAGGERED)) {
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
        (navigation_btn_pos == NAVIGATION_BTN_POS_AFTER ||
            navigation_btn_pos == NAVIGATION_BTN_POS_STAGGERED)) {
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
    WwtServices *services = wwt_app_get_services(self->app);

    if(services) {
        WindowManagerEvents *events =
            wwt_services_get_window_manager_events(services);
        window_manager_events_unsubscribe(events, self->events_subscription_id);
        self->events_subscription_id = -1;
    }

    if(self->wm_data) {
        window_manager_data_destroy(self->wm_data);
        self->wm_data = NULL;
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
    self->prev_focused_index = 0;
    setup_widgets(self);

    WwtServices *services = wwt_app_get_services(app);
    if(!services) {
        g_object_unref(self);
        return NULL;
    }

    WindowManagerEvents *wm_events =
        wwt_services_get_window_manager_events(services);

    self->events_subscription_id =
        window_manager_events_subscribe(wm_events, event_listener, self);

    wwt_taskbar_update_tabs(self);

    return self;
}
