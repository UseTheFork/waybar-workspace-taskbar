#include "window_manager_spec.h"
#include "window_managers/hyprland.h"
#include "window_managers/niri.h"
#include "window_managers/sway.h"

/**
 * Gets the events_constructor
 *
 * @param self
 * @return The events_constructor
 */
WindowManagerEventsConstructor window_manager_spec_get_events_constructor(
    WindowManagerSpec *self
) {
    return self->events_constructor;
}

/**
 * Gets the events_destructor
 *
 * @param self
 * @return The events_destructor
 */
WindowManagerEventsDestructor window_manager_spec_get_events_destructor(
    WindowManagerSpec *self
) {
    return self->events_destructor;
}

/**
 * Gets the events_reader
 *
 * @param self
 * @return The events_reader
 */
WindowManagerEventsReader window_manager_spec_get_events_reader(
    WindowManagerSpec *self
) {
    return self->events_reader;
}

/**
 * Gets the events_validator
 *
 * @param self
 * @return The events_validator
 */
WindowManagerEventsValidator window_manager_spec_get_events_validator(
    WindowManagerSpec *self
) {
    return self->events_validator;
}

/**
 * Gets the data_fetcher
 *
 * @param self
 * @return The data_fetcher
 */
WindowManagerDataFetcher window_manager_spec_get_data_fetcher(
    WindowManagerSpec *self
) {
    return self->data_fetcher;
}

/**
 * Destroys the window manager spec
 *
 * @param self
 */
void window_manager_spec_destroy(WindowManagerSpec *self) {
    g_free(self);
}

/**
 * Creates the window manager spec
 *
 * @param app The app instance
 * @return The fully created window manager spec
 */
WindowManagerSpec *window_manager_spec_create(WindowManagerId wm_id) {
    if(wm_id == WM_ID_NIRI) {
        return window_manager_spec_create_niri();
    }

    if(wm_id == WM_ID_HYPRLAND) {
        return window_manager_spec_create_hyprland();
    }

    if(wm_id == WM_ID_SWAY) {
        return window_manager_spec_create_sway();
    }

    return NULL;
}
