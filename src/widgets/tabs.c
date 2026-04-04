#include "tabs.h"
#include "core/app.h"
#include "core/window_manager.h"
#include "tab.h"
#include "window_managers/common.h"

struct _WwtTabs {
    GtkBox parent_instance;

    WwtApp *app;
};

G_DEFINE_TYPE(WwtTabs, wwt_tabs, GTK_TYPE_BOX);

/**
 * Handles the visual state after tab generation.
 *
 * @param tabs The tabs instance
 * @param tab_len How many tabs are in the bar
 */
void wwt_tabs_apply_visual_state(WwtTabs *tabs, int tab_len) {
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
}

/**
 * Generates the tabs
 *
 * @param bar The tabs instance
 */
gboolean wwt_tabs_generate_tabs(WwtTabs *self) {
    WwtWindowManager *wm = wwt_app_get_window_manager(self->app);
    WindowManagerGetWindows get_windows =
        wwt_window_manager_get_get_windows(wm);

    GPtrArray *wins =
        g_ptr_array_new_with_free_func((GDestroyNotify)wm_win_destroy);

    gboolean fetched = get_windows(self->app, wins);

    if (!fetched) {
        g_ptr_array_free(wins, TRUE);
        return FALSE;
    }

    GList *root = gtk_container_get_children(GTK_CONTAINER(self));

    for (guint i = 0; i < wins->len; i++) {
        WindowManagerWindow *win = g_ptr_array_index(wins, i);

        if (root == NULL) {
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
                WWT_TAB(root->data),
                win->id,
                win->title,
                win->app_id,
                win->focused,
                win->x,
                win->y
            );
        }

        if (root) {
            root = root->next;
        }
    }

    while (root != NULL) {
        GList *next = root->next;
        gtk_container_remove(GTK_CONTAINER(self), GTK_WIDGET(root->data));
        root = next;
    }

    gtk_widget_show_all(GTK_WIDGET(self));
    wwt_tabs_apply_visual_state(self, wins->len);
    g_ptr_array_free(wins, TRUE);

    return TRUE;
}

/**
 * Initialize the tabs instance
 *
 * @param self
 */
static void wwt_tabs_init(WwtTabs *self) {
}

/**
 * Dispose the tabs instance. Gref cleanup.
 *
 * @param obj The stuct obj.
 *
 */
static void wwt_tabs_dispose(GObject *obj) {
    WwtTabs *self = WWT_TABS(obj);

    printf("calling dispose on tabs\n");

    G_OBJECT_CLASS(wwt_tabs_parent_class)->dispose(obj);
}

/**
 * Finalizer for the tabs instance
 *
 * @param object The tabs struct
 */
static void wwt_tabs_finalize(GObject *obj) {
    WwtTabs *self = WWT_TABS(obj);

    printf("calling finalize on tabs\n");

    G_OBJECT_CLASS(wwt_tabs_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_tabs_class_init(WwtTabsClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = wwt_tabs_dispose;
    object_class->finalize = wwt_tabs_finalize;
}

/**
 * Creates a new instance of the tabs widget
 */
WwtTabs *wwt_tabs_new(WwtApp *app) {
    WwtTabs *self = g_object_new(
        WWT_TABS_TYPE,
        "orientation",
        GTK_ORIENTATION_HORIZONTAL,
        "spacing",
        0,
        NULL
    );

    self->app = app;

    return self;
}
