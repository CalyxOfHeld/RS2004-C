#ifndef RS2_GROUND225_H
#define RS2_GROUND225_H
#include "dash3d/world3d.h"

/* Isolated revision-225 ground entry points. Caller supplies the prepared
 * Pix3D palette/texture cache and bound surface. Click coordinates survive
 * a miss or disabled input, matching World3D's globals. */
typedef struct {
    int32_t eye_x,eye_y,eye_z,pitch,yaw;
    bool low_memory,taking_input;
    int32_t mouse_x,mouse_y,click_tile_x,click_tile_z;
} Ground225;
extern const int32_t world3d_texture_hsl225[50];
bool world3d_underlay225(const TileUnderlay *tile,const int32_t heights[4],
    int32_t tile_x,int32_t tile_z,Ground225 *view);
bool world3d_overlay225(const TileOverlay *tile,int32_t tile_x,int32_t tile_z,
    bool draw,Ground225 *view);
int32_t world3d_ground_lightness225(int32_t shade,int32_t texture_hsl);
bool world3d_point_inside_triangle225(int32_t mx,int32_t my,int32_t ya,int32_t yb,int32_t yc,
    int32_t xa,int32_t xb,int32_t xc);
#endif
