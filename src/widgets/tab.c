#include "tab.h"
#include "core/app.h"
#include "core/config.h"
#include "core/window_manager.h"
#include <gio/gdesktopappinfo.h>
#include <stdio.h>

#define TAB_CLASS_NAME "tab"
#define TAB_CLASS_NAME_FOCUSED "focused"
#define TITLE_MIN_CHARS 3

struct _WwtTab {
    GtkButton parent_instance;

    WwtApp *app;
    gchar *win_id;
    gchar *title;
    gchar *app_id;
    int focused;
    int x;
    int y;
};

G_DEFINE_TYPE(WwtTab, wwt_tab, GTK_TYPE_BUTTON);

/**
 * Trucats the title with an elipsis
 *
 * @param title The title to trucate
 * @param max_len The max characters for the string to be including elipsis
 */
static void truncate_title(gchar *title, int max_len) {
    if (max_len < TITLE_MIN_CHARS) {
        return;
    }

    int len = strlen(title);

    if (len > max_len && len > 3) {
        title[max_len] = '\0';
        title[max_len - 1] = '.';
        title[max_len - 2] = '.';
        title[max_len - 3] = '.';
    }
}

/**
 * Sets the buttons icon. First checks the gdesktopappinfo then goes to gtk
 * image from gicon
 *
 * @param self
 * @return TRUE if the icon is set else FALSE
 */
static gboolean set_btn_icon(WwtTab *self) {
    GDesktopAppInfo *info = g_desktop_app_info_new(self->app_id);

    if (!info) {
        gchar *desktop_id = g_strdup_printf("%s.desktop", self->app_id);
        info = g_desktop_app_info_new(desktop_id);
        g_free(desktop_id);
    }

    if (info) {
        GIcon *icon = g_app_info_get_icon(G_APP_INFO(info));
        GtkWidget *image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(self), image);
        gtk_button_set_always_show_image(GTK_BUTTON(self), TRUE);
        g_object_unref(info);
    }

    return TRUE;
}

/**
 * Sets up the button title, icon and alignment based on user config
 *
 * @param self
 */
static void setup_title_and_icon(WwtTab *self) {
    WwtApp *app = self->app;
    WwtConfig *config = wwt_app_get_config(app);

    gboolean show_title = wwt_config_get_show_title(config);
    gboolean show_icon = wwt_config_get_show_icon(config);
    int title_max_chars = wwt_config_get_title_max_chars(config);

    if (show_icon) {
        set_btn_icon(self);
        gtk_button_set_always_show_image(GTK_BUTTON(self), TRUE);
    }

    if (show_title) {
        truncate_title(self->title, title_max_chars);
        gtk_button_set_label(GTK_BUTTON(self), self->title);
    }
}

/**
 * Handle the button click event
 *
 * @param widget The button instance
 * @param event The button click event
 * @param user_data Null in this case
 * @return TRUE if the press was processed else FALSE
 */
static gboolean on_button_press(
    GtkWidget *widget,
    GdkEventButton *event,
    gpointer user_data
) {
    WwtTab *tab = WWT_TAB(widget);
    WwtWindowManager *wm = wwt_app_get_window_manager(tab->app);

    WindowManagerClickHandler window_focus =
        wwt_window_manager_get_click_handler(wm, WM_CLICK_FOCUS);
    WindowManagerClickHandler window_close =
        wwt_window_manager_get_click_handler(wm, WM_CLICK_CLOSE);
    WindowManagerClickHandler window_float =
        wwt_window_manager_get_click_handler(wm, WM_CLICK_FLOAT);

    if (event->button == 1) {
        window_focus(tab->win_id);
        return TRUE;
    }

    if (event->button == 2) {
        window_float(tab->win_id);
        return TRUE; // stop propagation
    }

    if (event->button == 3) {
        window_close(tab->win_id);
        // handle right click
        return TRUE;
    }

    return FALSE; // let GtkButton handle click normally
}

/**
 * Initialize the tab instance
 *
 * @param self
 */
static void wwt_tab_init(WwtTab *self) {
    gtk_widget_add_events(GTK_WIDGET(self), GDK_BUTTON_PRESS_MASK);
    g_signal_connect(
        self,
        "button-press-event",
        G_CALLBACK(on_button_press),
        NULL
    );
}

/**
 * Dispose the tab instance. Gref cleanup.
 *
 * @param obj The stuct obj.
 */
static void wwt_tab_dispose(GObject *obj) {
    G_OBJECT_CLASS(wwt_tab_parent_class)->dispose(obj);
}

/**
 * Finalizer for the tab instance
 *
 * @param object The tab struct
 */
static void wwt_tab_finalize(GObject *obj) {
    WwtTab *self = WWT_TAB(obj);

    g_free(self->win_id);
    g_free(self->title);
    g_free(self->app_id);

    G_OBJECT_CLASS(wwt_tab_parent_class)->finalize(obj);
}

/**
 * Class initializer
 *
 * @param klass the object class
 */
static void wwt_tab_class_init(WwtTabClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = wwt_tab_dispose;
    object_class->finalize = wwt_tab_finalize;
}

/**
 * Updates the tab information so it can be reused.
 *
 * @param self The tab instance
 * @param id The tabs id (should be the same as the compositor window)
 * @param name The window name
 * @param app_id The application id or class
 * @param ws_name The workspace name
 * @param focused The focused status
 * @param x Window position x
 * @param y Window position y
 */
void wwt_tab_update(
    WwtTab *self,
    const gchar *win_id,
    const gchar *name,
    const gchar *app_id,
    int focused,
    int x,
    int y
) {
    g_free(self->win_id);
    self->win_id = g_strdup(win_id);
    g_free(self->title);
    self->title = g_strdup(name);
    g_free(self->app_id);
    self->app_id = g_strdup(app_id);

    self->focused = focused;
    self->x = x;
    self->y = y;

    setup_title_and_icon(self);

    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
    if (self->focused) {
        gtk_style_context_add_class(ctx, TAB_CLASS_NAME_FOCUSED);
    } else {
        gtk_style_context_remove_class(ctx, TAB_CLASS_NAME_FOCUSED);
    }
}

/**
 * Creates a new instance of the tab widget
 *
 * @param tabs The tabs instance
 * @param id The tabs id (should be the same as the compositor window)
 * @param name The window name
 * @param app_id The application id or class
 * @param focused The focused status
 * @param x Window position x
 * @param y Window position y
 * @return The created tab widget
 */
WwtTab *wwt_tab_new(
    WwtApp *app,
    const gchar *win_id,
    const gchar *name,
    const gchar *app_id,
    int focused,
    int x,
    int y
) {
    WwtTab *self = g_object_new(WWT_TAB_TYPE, NULL);

    self->app = app;
    self->win_id = g_strdup(win_id);
    self->title = g_strdup(name);
    self->app_id = g_strdup(app_id);
    self->focused = focused;
    self->x = x;
    self->y = y;

    WwtConfig *config = wwt_app_get_config(app);

    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
    gtk_style_context_add_class(ctx, TAB_CLASS_NAME);

    if (self->focused) {
        gtk_style_context_add_class(ctx, TAB_CLASS_NAME_FOCUSED);
    }

    setup_title_and_icon(self);

    gfloat text_align = wwt_config_get_text_align(config);
    gtk_button_set_alignment(GTK_BUTTON(self), text_align, 0.5);

    return self;
}
