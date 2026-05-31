#include "subscriptions.h"

#define MAX_SUBSCRIPTIONS 20

typedef struct {
    WwtWindowManagerSubscriptionCallback cb;
    gpointer user_data;
} WindowManagerSubscription;

struct _WwtWindowManagerSubscriptions {
    GObject parent_instance;

    size_t count;
    WindowManagerSubscription *subs[MAX_SUBSCRIPTIONS];
};

G_DEFINE_TYPE(
    WwtWindowManagerSubscriptions,
    wwt_window_manager_subscriptions,
    G_TYPE_OBJECT
);

/**
 * Notify all subscribers with the data provided
 *
 * @param self
 * @param data The data to notify subscribers with
 */
void wwt_window_manager_subscriptions_notify(
    WwtWindowManagerSubscriptions *self,
    gpointer data
) {
    for(int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
        if(self->subs[i]) {
            self->subs[i]->cb(data, self->subs[i]->user_data);
        }
    }
}

/**
 * Subscribe to an event
 *
 * @param self
 * @param cb The function to fire
 * @param user_data Data to be passed to the function
 */
int wwt_window_manager_subscriptions_subscribe(
    WwtWindowManagerSubscriptions *self,
    WwtWindowManagerSubscriptionCallback cb,
    gpointer user_data
) {
    for(int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
        if(!self->subs[i]) {
            WindowManagerSubscription *sub =
                g_malloc(sizeof(WindowManagerSubscription));

            sub->cb = cb;
            sub->user_data = user_data;
            self->subs[i] = sub;
            self->count++;

            return i;
        }
    }
    return -1;
}

/**
 * Unsubscribe from an event
 *
 * @param self
 * @param id The subscription id
 */
void wwt_window_manager_subscriptions_unsubscribe(
    WwtWindowManagerSubscriptions *self,
    int id
) {
    if(!self || id < 0 || id >= MAX_SUBSCRIPTIONS) {
        return;
    }

    if(self->subs[id]) {
        g_free(self->subs[id]);
        self->subs[id] = NULL;
        self->count--;
    }
}

/**
 * Dispose the instance
 *
 * @param obj The instance object
 */
static void dispose(GObject *obj) {
    WwtWindowManagerSubscriptions *self = WWT_WINDOW_MANAGER_SUBSCRIPTIONS(obj);

    if(self->count) {
        for(int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
            if(self->subs[i]) {
                wwt_window_manager_subscriptions_unsubscribe(self, i);
            }
        }
    }

    G_OBJECT_CLASS(wwt_window_manager_subscriptions_parent_class)->dispose(obj);
}

/**
 * Initialize the instance
 *
 * @param obj The instance object
 */
static void wwt_window_manager_subscriptions_init(
    WwtWindowManagerSubscriptions *self
) {
    self->count = 0;
}

/**
 * Initialize the class
 *
 * @param klass The class instance
 */
static void wwt_window_manager_subscriptions_class_init(
    WwtWindowManagerSubscriptionsClass *klass
) {
    G_OBJECT_CLASS(klass)->dispose = dispose;
}

/**
 * Creates a new instance
 *
 * @return self
 */
WwtWindowManagerSubscriptions *wwt_window_manager_subscriptions_new() {
    WwtWindowManagerSubscriptions *self =
        g_object_new(WWT_WINDOW_MANAGER_SUBSCRIPTIONS_TYPE, NULL);

    return self;
}
