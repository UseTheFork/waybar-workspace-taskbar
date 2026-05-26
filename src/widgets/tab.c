#include "tab.h"
#include "core/app.h"
#include "core/config.h"
#include "core/services.h"
#include "services/app_icons.h"
#include "utils/cmd.h"
#include "utils/common.h"
#include <gio/gdesktopappinfo.h>

#define TAB_CLASS_NAME "tab"
#define TAB_CLASS_NAME_FOCUSED "focused"
#define TAB_CLASS_NAME_FLOATING "floating"
#define TAB_CLASS_NAME_URGENT "urgent"

struct _WwtTab {
    GtkButton parent_instance;

    WwtApp *app;
    gchar *win_id;
    gchar *title;
    gchar *app_id;
    int focused;
    int floating;
    int urgent;
    int x;
    int y;
};

G_DEFINE_TYPE(WwtTab, wwt_tab, GTK_TYPE_BUTTON);

/**
 * Sets the buttons icon. First checks the gdesktopappinfo then goes to gtk
 * image from gicon
 *
 * @param self
 * @return TRUE if the icon is set else FALSE
 */
static gboolean set_btn_icon(WwtTab *self) {
    WwtServices *services = wwt_app_get_services(self->app);
    WwtConfig *config = wwt_app_get_config(self->app);
    AppIcons *app_icons = wwt_services_get_app_icons(services);

    if(!services || !config || !app_icons) {
        return FALSE;
    }

    int icon_size = wwt_config_get_icon_size(config);
    GdkPixbuf *pixbuf = app_icons_get_icon(app_icons, self->app_id, icon_size);
    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);

    gtk_button_set_image(GTK_BUTTON(self), image);
    gtk_button_set_always_show_image(GTK_BUTTON(self), TRUE);

    return TRUE;
}

/**
 * Apply the class names based on the window state
 *
 * @parma self
 */
static void apply_class_names(WwtTab *self) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));

    if(self->focused) {
        gtk_style_context_add_class(ctx, TAB_CLASS_NAME_FOCUSED);

    } else {
        gtk_style_context_remove_class(ctx, TAB_CLASS_NAME_FOCUSED);
    }

    if(self->floating) {
        gtk_style_context_add_class(ctx, TAB_CLASS_NAME_FLOATING);

    } else {
        gtk_style_context_remove_class(ctx, TAB_CLASS_NAME_FLOATING);
    }

    if(self->urgent) {
        gtk_style_context_add_class(ctx, TAB_CLASS_NAME_URGENT);

    } else {
        gtk_style_context_remove_class(ctx, TAB_CLASS_NAME_URGENT);
    }
}

/**
 * Truncates the title with an ellipsis for the label
 *
 * @param title The title to truncate
 * @param len The max characters for the string including ellipsis
 */
static char *ellipsis_title(gchar *title, int len) {
    if(len <= CONFIG_TITLE_MIN_CHARS) {
        return g_strdup(title);
    }

    glong str_len = g_utf8_strlen(title, -1);
    if(str_len <= len) {
        return g_strdup(title);
    }

    const gchar *end = g_utf8_offset_to_pointer(title, len - 3);
    GString *str = g_string_new_len(title, end - title);
    g_string_append(str, "...");

    return g_string_free(str, FALSE);
}

/**
 * Sets up the button title, icon and alignment based on user config
 *
 * @param self
 */
static void set_title_and_icon(WwtTab *self) {
    WwtApp *app = self->app;
    WwtConfig *config = wwt_app_get_config(app);

    gboolean show_title = wwt_config_get_show_title(config);
    gboolean show_icon = wwt_config_get_show_icon(config);
    gboolean show_tooltip = wwt_config_get_show_tooltip(config);
    int title_max_chars = wwt_config_get_title_max_chars(config);

    if(show_icon) {
        set_btn_icon(self);
    }

    if(show_tooltip) {
        gtk_widget_set_tooltip_text(GTK_WIDGET(self), self->title);
    }

    if(show_title) {
        if(title_max_chars > CONFIG_TITLE_MIN_CHARS) {
            g_autofree gchar *title =
                ellipsis_title(self->title, title_max_chars);
            gtk_button_set_label(GTK_BUTTON(self), title);
        } else {
            gtk_button_set_label(GTK_BUTTON(self), self->title);
        }
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
    WwtTab *self = WWT_TAB(widget);
    WwtConfig *config = wwt_app_get_config(self->app);

    // Left click
    if(event->button == 1) {
        const char *on_click = wwt_config_get_on_click(config);

        if(on_click) {
            g_autofree gchar *cmd = str_replace(on_click, "{id}", self->win_id);
            cmd_run_fork_exec(cmd);
        }
    }

    // Middle click
    if(event->button == 2) {
        const char *on_click_middle = wwt_config_get_on_click_middle(config);

        if(on_click_middle) {
            g_autofree gchar *cmd =
                str_replace(on_click_middle, "{id}", self->win_id);
            cmd_run_fork_exec(cmd);
        }
    }

    // Right click
    if(event->button == 3) {
        const char *on_click_right = wwt_config_get_on_click_right(config);

        if(on_click_right) {
            g_autofree gchar *cmd =
                str_replace(on_click_right, "{id}", self->win_id);
            cmd_run_fork_exec(cmd);
        }
    }

    return TRUE; // Stop propogation
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
 * @param title The window title
 * @param app_id The application id or class
 * @param ws_name The workspace name
 * @param focused The focused status
 * @param floating The floating status
 * @param urgent The urgent status
 * @param x Window position x
 * @param y Window position y
 */
void wwt_tab_update(
    WwtTab *self,
    const gchar *win_id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y
) {
    g_free(self->win_id);
    self->win_id = g_strdup(win_id);
    g_free(self->title);
    self->title = title ? g_strdup(title) : g_strdup("");
    g_free(self->app_id);
    self->app_id = app_id ? g_strdup(app_id) : g_strdup("");

    self->focused = focused;
    self->floating = floating;
    self->urgent = urgent;
    self->x = x;
    self->y = y;

    apply_class_names(self);
    set_title_and_icon(self);
}

/**
 * Creates a new instance of the tab widget
 *
 * @param tabs The tabs instance
 * @param id The tabs id (should be the same as the compositor window)
 * @param title The window title
 * @param app_id The application id or class
 * @param focused The focused status
 * @param floating The floating status
 * @param urgent The urgent status
 * @param x Window position x
 * @param y Window position y
 * @return The created tab widget
 */
WwtTab *wwt_tab_new(
    WwtApp *app,
    const gchar *win_id,
    const gchar *title,
    const gchar *app_id,
    int focused,
    int floating,
    int urgent,
    int x,
    int y
) {
    WwtTab *self = g_object_new(WWT_TAB_TYPE, NULL);

    self->app = app;
    self->win_id = g_strdup(win_id);
    self->title = title ? g_strdup(title) : g_strdup("");
    self->app_id = app_id ? g_strdup(app_id) : g_strdup("");
    self->focused = focused;
    self->floating = floating;
    self->urgent = urgent;
    self->x = x;
    self->y = y;

    WwtConfig *config = wwt_app_get_config(app);

    GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(self));
    gtk_style_context_add_class(ctx, TAB_CLASS_NAME);

    apply_class_names(self);
    set_title_and_icon(self);

    gfloat text_align = wwt_config_get_text_align(config);
    gtk_button_set_alignment(GTK_BUTTON(self), text_align, 0.5);

    return self;
}
