#ifndef RS2_VISIBILITY225_H
#define RS2_VISIBILITY225_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t left,top,right,bottom,center_x,center_y;
    int32_t sin_yaw,cos_yaw,sin_pitch,cos_pitch;
} VisibilityProjection225;
typedef struct {
    VisibilityProjection225 projection;
    bool matrix[8][32][51][51];
} Visibility225;

/* Isolated World3D.init/testPoint port. Initialize the context to zero once;
 * subsequent calls preserve the last matrix row/column, as Java does.
 * Requires initialized Pix3D trig tables. Height extents are bounded to
 * 0..32767; original client uses above=800, below=500. */
bool world3d_visibility_init225(Visibility225 *state,const int32_t distance[9],
    int32_t above,int32_t width,int32_t height,int32_t below);
bool world3d_visibility_point225(const VisibilityProjection225 *projection,
    int32_t x,int32_t z,int32_t y);
#endif
