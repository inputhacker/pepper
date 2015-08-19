#include "desktop-shell-internal.h"
#include <stdlib.h>

void
shell_get_output_workarea(desktop_shell_t       *shell,
                          pepper_output_t       *output,
                          pixman_rectangle32_t  *area)
{
    const pepper_output_geometry_t *geom;

    /**
     ** TODO: Get given output's workarea size and position in global coordinate
     **      return (output_size - (panel_size + margin + caption + ... ));
     **/

    geom = pepper_output_get_geometry(output);

    if (area)
    {
        area->x = geom->x;
        area->y = geom->y;
        area->width = geom->w;
        area->height = geom->h;
    }
}

static void
handle_shell_client_destroy(struct wl_listener *listener, void *data)
{
    shell_client_t *shell_client = pepper_container_of(listener,
                                                       shell_client_t,
                                                       client_destroy_listener);

    remove_ping_timer(shell_client);

    wl_list_remove(&shell_client->link);

    free(shell_client);
}

shell_client_t *
shell_client_create(desktop_shell_t *shell, struct wl_client *client,
                    const struct wl_interface *interface, const void *implementation,
                    uint32_t version, uint32_t id)
{
    shell_client_t  *shell_client;

    shell_client = calloc(1, sizeof(shell_client_t));
    if (!shell_client)
    {
        wl_client_post_no_memory(client);
        return NULL;
    }

    shell_client->resource = wl_resource_create(client, interface, version, id);
    if (!shell_client->resource)
    {
        wl_client_post_no_memory(client);
        free(shell_client);
        return NULL;
    }

    shell_client->shell  = shell;
    shell_client->client = client;

    shell_client->client_destroy_listener.notify = handle_shell_client_destroy;
    wl_client_add_destroy_listener(client, &shell_client->client_destroy_listener);

    wl_list_insert(&shell->shell_client_list, &shell_client->link);

    wl_resource_set_implementation(shell_client->resource, implementation, shell_client, NULL);

    return shell_client;
}

static void
input_device_add_callback(pepper_event_listener_t    *listener,
                          pepper_object_t            *object,
                          uint32_t                    id,
                          void                       *info,
                          void                       *data)
{
    desktop_shell_t         *shell = (desktop_shell_t *)data;
    pepper_input_device_t   *device = info;
    shell_seat_t            *shseat;
    const char              *target_seat_name;
    const char              *seat_name;
    pepper_list_t           *l;

    target_seat_name = pepper_input_device_get_property(device, "seat_name");
    if (!target_seat_name)
        target_seat_name = "seat0";

    PEPPER_LIST_FOR_EACH(&shell->shseat_list, l)
    {
        shseat = l->item;

        seat_name = pepper_seat_get_name(shseat->seat);

        /* Find seat to adding input device */
        if ( seat_name && !strcmp(seat_name, target_seat_name))
        {
            /* TODO: How to check this device was already attached? */
            pepper_seat_add_input_device(shseat->seat, device);
            return ;
        }
    }

    shseat = calloc(1, sizeof(shell_seat_t));
    if (!shseat)
    {
        PEPPER_ERROR("Memory allocation faiiled\n");
        return ;
    }

    /* Add a new seat to compositor */
    shseat->seat = pepper_compositor_add_seat(shell->compositor, target_seat_name, NULL);
    if (!shseat->seat)
    {
        PEPPER_ERROR("pepper_compositor_add_seat failed\n");
        free(shseat);
        return ;
    }

    shseat->shell = shell;

    pepper_list_insert(&shell->shseat_list, &shseat->link);

    /* Add this input_device to seat */
    pepper_seat_add_input_device(shseat->seat, device);
}

static void
event_handler_callback(pepper_event_listener_t    *listener,
                       pepper_object_t            *object,
                       uint32_t                    id,
                       void                       *info,
                       void                       *data)
{
    /* TODO */
}

static void
seat_logical_device_add_callback(pepper_event_listener_t    *listener,
                                 pepper_object_t            *object,
                                 uint32_t                    id,
                                 void                       *info,
                                 void                       *data)
{
    shell_seat_t *shseat = data;

    switch (id)
    {
    case PEPPER_EVENT_SEAT_POINTER_ADD:
        {
            pepper_object_t *pointer = info;

            shseat->pointer_motion_listener =
                pepper_object_add_event_listener(pointer, PEPPER_EVENT_POINTER_MOTION,
                                                 0, event_handler_callback, shseat);

            shseat->pointer_button_listener =
                pepper_object_add_event_listener(pointer, PEPPER_EVENT_POINTER_BUTTON,
                                                 0, event_handler_callback, shseat);

            shseat->pointer_axis_listener =
                pepper_object_add_event_listener(pointer, PEPPER_EVENT_POINTER_AXIS,
                                                 0, event_handler_callback, shseat);
        }
        break;
    case PEPPER_EVENT_SEAT_KEYBOARD_ADD:
        {
            pepper_object_t *keyboard = info;

            shseat->keyboard_key_listener =
                pepper_object_add_event_listener(keyboard, PEPPER_EVENT_KEYBOARD_KEY,
                                                 0, event_handler_callback, shseat);

            shseat->keyboard_modifiers_listener =
                pepper_object_add_event_listener(keyboard, PEPPER_EVENT_KEYBOARD_MODIFIERS,
                                                 0, event_handler_callback, shseat);
        }
        break;
    case PEPPER_EVENT_SEAT_TOUCH_ADD:
        {
            pepper_object_t *touch = info;

            shseat->touch_down_listener =
                pepper_object_add_event_listener(touch, PEPPER_EVENT_TOUCH_DOWN,
                                                 0, event_handler_callback, shseat);

            shseat->touch_up_listener =
                pepper_object_add_event_listener(touch, PEPPER_EVENT_TOUCH_UP,
                                                 0, event_handler_callback, shseat);

            shseat->touch_motion_listener =
                pepper_object_add_event_listener(touch, PEPPER_EVENT_TOUCH_MOTION,
                                                 0, event_handler_callback, shseat);

            shseat->touch_frame_listener =
                pepper_object_add_event_listener(touch, PEPPER_EVENT_TOUCH_FRAME,
                                                 0, event_handler_callback, shseat);

            shseat->touch_cancel_listener =
                pepper_object_add_event_listener(touch, PEPPER_EVENT_TOUCH_CANCEL,
                                                 0, event_handler_callback, shseat);
        }
        break;
    default :
        PEPPER_ERROR("unknown event %d\n", id);
    }
}

static void
seat_logical_device_remove_callback(pepper_event_listener_t    *listener,
                                    pepper_object_t            *object,
                                    uint32_t                    id,
                                    void                       *info,
                                    void                       *data)
{
    shell_seat_t *shseat = data;

    switch (id)
    {
    case PEPPER_EVENT_SEAT_POINTER_REMOVE:
        {
            pepper_event_listener_remove(shseat->pointer_motion_listener);
            shseat->pointer_motion_listener  = NULL;

            pepper_event_listener_remove(shseat->pointer_button_listener);
            shseat->pointer_button_listener = NULL;

            pepper_event_listener_remove(shseat->pointer_axis_listener);
            shseat->pointer_axis_listener    = NULL;
        }
        break;
    case PEPPER_EVENT_SEAT_KEYBOARD_REMOVE:
        {
            pepper_event_listener_remove(shseat->keyboard_key_listener);
            shseat->keyboard_key_listener = NULL;

            pepper_event_listener_remove(shseat->keyboard_modifiers_listener);
            shseat->keyboard_modifiers_listener = NULL;
        }
        break;
    case PEPPER_EVENT_SEAT_TOUCH_REMOVE:
        {
            pepper_event_listener_remove(shseat->touch_down_listener);
            shseat->touch_down_listener = NULL;

            pepper_event_listener_remove(shseat->touch_up_listener);
            shseat->touch_up_listener = NULL;

            pepper_event_listener_remove(shseat->touch_motion_listener);
            shseat->touch_motion_listener = NULL;

            pepper_event_listener_remove(shseat->touch_frame_listener);
            shseat->touch_frame_listener = NULL;

            pepper_event_listener_remove(shseat->touch_cancel_listener);
            shseat->touch_cancel_listener = NULL;
        }
        break;
    default :
        PEPPER_ERROR("unknown event %d\n", id);
    }
}

static void
seat_add_callback(pepper_event_listener_t    *listener,
                  pepper_object_t            *object,
                  uint32_t                    id,
                  void                       *info,
                  void                       *data)
{
    desktop_shell_t         *shell = data;
    pepper_seat_t           *seat  = info;
    shell_seat_t            *shseat;
    pepper_list_t           *l;
    pepper_pointer_t        *pointer;
    pepper_keyboard_t       *keyboard;
    pepper_touch_t          *touch;

    PEPPER_LIST_FOR_EACH(&shell->shseat_list, l)
    {
        shseat = l->item;

        if (shseat->seat == seat)
            return ;
    }

    shseat = calloc(1, sizeof(shell_seat_t));
    if (!shseat)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return ;
    }
    shseat->seat  = seat;
    shseat->shell = shell;

    pepper_list_insert(&shell->shseat_list, &shseat->link);

    pointer = pepper_seat_get_pointer(seat);
    if (pointer)
    {
        shseat->pointer_motion_listener =
            pepper_object_add_event_listener((pepper_object_t *)pointer, PEPPER_EVENT_POINTER_MOTION,
                                             0, event_handler_callback, shseat);

        shseat->pointer_button_listener =
            pepper_object_add_event_listener((pepper_object_t *)pointer, PEPPER_EVENT_POINTER_BUTTON,
                                             0, event_handler_callback, shseat);

        shseat->pointer_axis_listener =
            pepper_object_add_event_listener((pepper_object_t *)pointer, PEPPER_EVENT_POINTER_AXIS,
                                             0, event_handler_callback, shseat);
    }
    else
    {
        shseat->pointer_add_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_POINTER_ADD,
                                             0, seat_logical_device_add_callback, shseat);
        shseat->pointer_remove_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_POINTER_REMOVE,
                                             0, seat_logical_device_remove_callback, shseat);
    }

    keyboard = pepper_seat_get_keyboard(seat);
    if (keyboard)
    {
        shseat->keyboard_key_listener =
            pepper_object_add_event_listener((pepper_object_t *)keyboard, PEPPER_EVENT_KEYBOARD_KEY,
                                             0, event_handler_callback, shseat);
        shseat->keyboard_modifiers_listener =
            pepper_object_add_event_listener((pepper_object_t *)keyboard, PEPPER_EVENT_KEYBOARD_MODIFIERS,
                                             0, event_handler_callback, shseat);
    }
    else
    {
        shseat->keyboard_add_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_KEYBOARD_ADD,
                                             0, seat_logical_device_add_callback, shseat);
        shseat->keyboard_remove_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_KEYBOARD_REMOVE,
                                             0, seat_logical_device_remove_callback, shseat);
    }

    touch = pepper_seat_get_touch(seat);
    if (touch)
    {
        shseat->touch_down_listener =
            pepper_object_add_event_listener((pepper_object_t *)touch, PEPPER_EVENT_TOUCH_DOWN,
                                             0, event_handler_callback, shseat);
        shseat->touch_up_listener =
            pepper_object_add_event_listener((pepper_object_t *)touch, PEPPER_EVENT_TOUCH_UP,
                                             0, event_handler_callback, shseat);
        shseat->touch_motion_listener =
            pepper_object_add_event_listener((pepper_object_t *)touch, PEPPER_EVENT_TOUCH_MOTION,
                                             0, event_handler_callback, shseat);
        shseat->touch_frame_listener =
            pepper_object_add_event_listener((pepper_object_t *)touch, PEPPER_EVENT_TOUCH_FRAME,
                                             0, event_handler_callback, shseat);
        shseat->touch_cancel_listener =
            pepper_object_add_event_listener((pepper_object_t *)touch, PEPPER_EVENT_TOUCH_CANCEL,
                                             0, event_handler_callback, shseat);
    }
    else
    {
        shseat->touch_add_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_TOUCH_ADD,
                                             0, seat_logical_device_add_callback, shseat);
        shseat->touch_remove_listener =
            pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_TOUCH_REMOVE,
                                             0, seat_logical_device_add_callback, shseat);
    }
}

static void
seat_remove_callback(pepper_event_listener_t    *listener,
                     pepper_object_t            *object,
                     uint32_t                    id,
                     void                       *info,
                     void                       *data)
{
    desktop_shell_t         *shell = data;
    pepper_seat_t           *seat  = info;
    shell_seat_t            *shseat;
    pepper_list_t           *l;

    PEPPER_LIST_FOR_EACH(&shell->shseat_list, l)
    {
        shseat = l->item;

        if (shseat->seat == seat)
        {
            pepper_list_remove(&shseat->link, NULL);

            free(shseat);
            return ;
        }
    }
}

static void
init_listeners(desktop_shell_t *shell)
{
    pepper_object_t *compositor = (pepper_object_t *)shell->compositor;

    /* input_device_add */
    shell->input_device_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
                                         0, input_device_add_callback, shell);

    shell->seat_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
                                         0, seat_add_callback, shell);

    shell->seat_remove_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
                                         0, seat_remove_callback, shell);
}

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_compositor_t *compositor)
{
    desktop_shell_t *shell;

    shell = calloc(1, sizeof(desktop_shell_t));
    if (!shell)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return PEPPER_FALSE;
    }

    shell->compositor = compositor;

    wl_list_init(&shell->shell_client_list);
    wl_list_init(&shell->shell_surface_list);

    if (!init_wl_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        free(shell);
        return PEPPER_FALSE;
    }

    if (!init_xdg_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        fini_wl_shell(shell);
        free(shell);
        return PEPPER_FALSE;
    }

    init_listeners(shell);

    return PEPPER_TRUE;
}
