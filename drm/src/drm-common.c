#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>
#include <gbm.h>

#include "drm-internal.h"

PEPPER_API pepper_drm_t *
pepper_drm_create(pepper_compositor_t *compositor, const char *device, const char *renderer)
{
    pepper_drm_t    *drm;

    drm = (pepper_drm_t *)calloc(1, sizeof(pepper_drm_t));
    if (!drm)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    drm->udev = udev_new();
    if (!drm->udev)
    {
        PEPPER_ERROR("Failed to create udev context in %s\n", __FUNCTION__);
        goto error;
    }

    drm->compositor = compositor;
    drm->input = pepper_libinput_create(compositor, drm->udev);

    if (!drm->input)
    {
        PEPPER_ERROR("Failed to create pepper_libinput in %s\n", __FUNCTION__);
        goto error;
    }

    wl_list_init(&drm->output_list);

    if (!pepper_drm_output_create(drm, renderer))
    {
        PEPPER_ERROR("Failed to connect drm output in %s\n", __FUNCTION__);
        goto error;
    }

    return drm;

error:
    if (drm)
        pepper_drm_destroy(drm);

    return NULL;
}

PEPPER_API void
pepper_drm_destroy(pepper_drm_t *drm)
{
    drm_output_t *output, *next;

    if (!drm)
        return;

    if (drm->renderer)
        free(drm->renderer);

    if (drm->udev_monitor_source)
        wl_event_source_remove(drm->udev_monitor_source);

    if (drm->udev_monitor)
        udev_monitor_unref(drm->udev_monitor);

    if (drm->drm_event_source)
        wl_event_source_remove(drm->drm_event_source);

    if (!wl_list_empty(&drm->output_list))
    {
        wl_list_for_each_safe(output, next, &drm->output_list, link)
            pepper_drm_output_destroy(output);
    }

    if (drm->crtcs)
        free(drm->crtcs);

    if (drm->gbm_device)
        gbm_device_destroy(drm->gbm_device);

    if (drm->drm_fd)
        close(drm->drm_fd);

    if (drm->input)
        pepper_libinput_destroy(drm->input);

    if (drm->udev)
        udev_unref(drm->udev);

    free(drm);
}
