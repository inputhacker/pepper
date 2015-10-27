#include "drm-internal.h"

void
drm_init_planes(pepper_drm_t *drm)
{
    int                 i;
    drmModePlaneRes    *res = drmModeGetPlaneResources(drm->fd);

    PEPPER_CHECK(res, return, "drmModeGetPlaneResources() failed.\n");

    for (i = 0; i < (int)res->count_planes; i++)
    {
        drm_plane_t *plane = calloc(1, sizeof(drm_plane_t));
        PEPPER_CHECK(plane, continue, "calloc() failed.\n");

        plane->plane = drmModeGetPlane(drm->fd, res->planes[i]);
        if (!plane->plane)
        {
            PEPPER_ERROR("drmModeGetPlane() failed.\n");
            free(plane);
            continue;
        }
        plane->drm = drm;
        plane->id = plane->plane->plane_id;

        pepper_list_insert(drm->plane_list.prev, &plane->link);
    }

    drmModeFreePlaneResources(res);
}

void
drm_plane_destroy(drm_plane_t *plane)
{
    if (plane->plane)
        drmModeFreePlane(plane->plane);

    pepper_list_remove(&plane->link);
    free(plane);
}
