#ifndef RS2_WORLD3D_H
#define RS2_WORLD3D_H

#include <stdint.h>
#include <stdbool.h>
#include "gfx/model.h"

#define WORLD3D_MAX_LEVEL 4
#define WORLD3D_MAX_X 104
#define WORLD3D_MAX_Z 104

typedef struct {
    int southwest_color;
    int southeast_color;
    int northeast_color;
    int northwest_color;
    int texture_id;
    bool flat;
    int rgb;
} TileUnderlay;

typedef struct {
    int vertex_count;
    int vertex_x[10];
    int vertex_y[10];
    int vertex_z[10];
    int triangle_count;
    int triangle_vertex_a[10];
    int triangle_vertex_b[10];
    int triangle_vertex_c[10];
    int triangle_color_a[10];
    int triangle_color_b[10];
    int triangle_color_c[10];
    int triangle_texture_id;
    int triangle_texture[10];
    int shape;
    int rotation;
    int background_rgb;
    int foreground_rgb;
    bool flat;
} TileOverlay;

typedef struct {
    int level;
    int y;
    int x;
    int z;
    Model *model;
    int bitset;
    int shape;
    bool owns_model;
    int yaw;
    int min_tile_x;
    int max_tile_x;
    int min_tile_z;
    int max_tile_z;
} WorldLocation;

typedef struct {
    int level;
    int x;
    int z;
    int draw_level;
    TileUnderlay underlay;
    bool has_underlay;
    TileOverlay overlay;
    bool has_overlay;
    int loc_count;
    WorldLocation locs[5];
} GroundTile;

typedef struct {
    int level,x,z,id,shape,rotation,width,length,mapscene,mapfunction;
    bool active;
} WorldMapLocation;

typedef struct {
    int max_tile_x;
    int max_tile_z;
    int max_level;
    int min_level;
    int heightmap[WORLD3D_MAX_LEVEL][WORLD3D_MAX_X + 1][WORLD3D_MAX_Z + 1];
    GroundTile tiles[WORLD3D_MAX_LEVEL][WORLD3D_MAX_X][WORLD3D_MAX_Z];

    WorldMapLocation *map_locations;
    size_t map_location_count,map_location_capacity;

    struct MousePicker *picker;
    bool taking_input;
    int mouse_x;
    int mouse_y;
    int click_tile_x;
    int click_tile_z;
} World3D;

void world3d_init(World3D *w, int max_x, int max_z, int max_level);
void world3d_reset(World3D *w);
void world3d_dispose(World3D *w);
bool world3d_add_map_location(World3D *w,WorldMapLocation loc);
void world3d_click(World3D *w, int mouse_x, int mouse_y);
void world3d_set_tile(World3D *w, int level, int x, int z, int shape, int rotation, int texture_id,
                      int y_sw, int y_se, int y_ne, int y_nw,
                      int hsl_sw, int hsl_se, int hsl_ne, int hsl_nw,
                      int ovl_hsl_sw, int ovl_hsl_se, int ovl_hsl_ne, int ovl_hsl_nw,
                      int rgb_bg, int rgb_fg);
bool world3d_add_loc(World3D *w, int level, int x, int z, int y, Model *model, int bitset,
                     int width, int length);
void world3d_render(World3D *w, int eyeX, int eyeY, int eyeZ, int pitch, int yaw, int roll, int plane);

#endif // RS2_WORLD3D_H
