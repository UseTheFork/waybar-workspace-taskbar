#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _WindowManagerSubscriptions WindowManagerSubscriptions;

#define WWT_WINDOW_MANAGER_SUBSCRIPTIONS_TYPE \
    (wwt_window_manager_subscriptions_get_type())

G_DECLARE_FINAL_TYPE(
    WwtWindowManagerSubscriptions,
    wwt_window_manager_subscriptions,
    WWT,
    WINDOW_MANAGER_SUBSCRIPTIONS,
    GObject
);

typedef void (*WwtWindowManagerSubscriptionCallback)(
    gpointer data,
    gpointer user_data
);

WwtWindowManagerSubscriptions *wwt_window_manager_subscriptions_new();
int wwt_window_manager_subscriptions_subscribe(
    WwtWindowManagerSubscriptions *self,
    WwtWindowManagerSubscriptionCallback callback,
    gpointer user_data
);
void wwt_window_manager_subscriptions_unsubscribe(
    WwtWindowManagerSubscriptions *self,
    int subscription_id
);
void wwt_window_manager_subscriptions_notify(
    WwtWindowManagerSubscriptions *self,
    gpointer data
);

G_END_DECLS
