#include "pepper-internal.h"

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_impl =
{
    keyboard_release,
};

static void
default_keyboard_grab_key(pepper_keyboard_t *keyboard, void *data,
                          uint32_t time, uint32_t key, uint32_t state)
{
    pepper_keyboard_send_key(keyboard, time, key, state);
}


static void
default_keyboard_grab_cancel(pepper_keyboard_t *keyboard, void *data)
{
    /* Nothing to do. */
}

static const pepper_keyboard_grab_t default_keyboard_grab =
{
    default_keyboard_grab_key,
    default_keyboard_grab_cancel,
};

void
pepper_keyboard_handle_event(pepper_keyboard_t *keyboard, uint32_t id, pepper_input_event_t *event)
{
    uint32_t               *keys = keyboard->keys.data;
    unsigned int            num_keys = keyboard->keys.size / sizeof(uint32_t);
    unsigned int            i;

    if (id != PEPPER_EVENT_KEYBOARD_KEY)
        return;

    /* Update the array of pressed keys. */
    for (i = 0; i < num_keys; i++)
    {
        if (keys[i] == event->key)
        {
            keys[i] = keys[--num_keys];
            break;
        }
    }

    keyboard->keys.size = num_keys * sizeof(uint32_t);

    if (event->state == PEPPER_KEY_STATE_PRESSED)
        *(uint32_t *)wl_array_add(&keyboard->keys, sizeof(uint32_t)) = event->key;

    keyboard->grab->key(keyboard, keyboard->data, event->time, event->key, event->state);
    pepper_object_emit_event(&keyboard->base, id, event);
}

static void
keyboard_handle_focus_destroy(pepper_object_t *object, void *data)
{
    pepper_keyboard_t *keyboard = (pepper_keyboard_t *)object;

    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);
}

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat)
{
    pepper_keyboard_t *keyboard =
        (pepper_keyboard_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_keyboard_t));

    PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&keyboard->input, seat, &keyboard->base, keyboard_handle_focus_destroy);
    keyboard->grab = &default_keyboard_grab;

    wl_array_init(&keyboard->keys);
    return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);

    pepper_input_fini(&keyboard->input);
    wl_array_release(&keyboard->keys);
    free(keyboard);
}

void
pepper_keyboard_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_keyboard_t  *keyboard = seat->keyboard;
    struct wl_resource *res;

    if (!keyboard)
        return;

    res = pepper_input_bind_resource(&keyboard->input, client, wl_resource_get_version(resource),
                                     id, &wl_keyboard_interface, &keyboard_impl, keyboard);
    PEPPER_CHECK(res, return, "pepper_input_bind_resource() failed.\n");

    if (!keyboard->input.focus)
        return;

    if (wl_resource_get_client(keyboard->input.focus->surface->resource) == client)
    {
        wl_keyboard_send_enter(res, keyboard->input.focus_serial,
                              keyboard->input.focus->surface->resource, &keyboard->keys);
    }
}

PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus)
{
    pepper_keyboard_send_leave(keyboard);
    pepper_input_set_focus(&keyboard->input, focus);
    pepper_keyboard_send_enter(keyboard);
}

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
    return keyboard->input.focus;
}

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_leave(resource, serial, keyboard->input.focus->surface->resource);
    }
}

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
        {
            wl_keyboard_send_enter(resource, serial, keyboard->input.focus->surface->resource,
                                   &keyboard->keys);
        }
    }
}

PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, uint32_t time, uint32_t key, uint32_t state)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_key(resource, serial, time, key, state);
    }
}

PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, uint32_t depressed, uint32_t latched,
                               uint32_t locked, uint32_t group)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_modifiers(resource, serial, depressed, latched, locked, group);
    }
}

PEPPER_API void
pepper_keyboard_start_grab(pepper_keyboard_t *keyboard,
                           const pepper_keyboard_grab_t *grab, void *data)
{
    keyboard->grab = grab;
    keyboard->data = data;
}

PEPPER_API void
pepper_keyboard_end_grab(pepper_keyboard_t *keyboard)
{
    keyboard->grab = &default_keyboard_grab;
}
