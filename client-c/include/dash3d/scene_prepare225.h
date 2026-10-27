#ifndef RS2_SCENE_PREPARE225_H
#define RS2_SCENE_PREPARE225_H
#include "dash3d/scene_traversal225.h"
#include "dash3d/visibility225.h"

/* Original draw camera setup, active occluders and tile eligibility.
 * Caller selects the top-level occluder pool and retains graph/cycle state
 * across frames. No captured per-frame traversal state is consumed. */
bool world3d_prepare_scene225(SceneTraversal225 *scene,const Visibility225 *visibility,
    int32_t eye_x,int32_t eye_y,int32_t eye_z,int32_t pitch,int32_t yaw,int32_t top_level);
#endif
