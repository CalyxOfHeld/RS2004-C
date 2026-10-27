#ifndef RS2_MINIMAP_H
#define RS2_MINIMAP_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "gfx/pix32.h"
#include "gfx/media.h"
#include "dash3d/world.h"
#include "config/flo.h"
#include "entity/npc_entity.h"
#include "entity/player_entity.h"

typedef struct {
    const World3D *scene;
    int level;
    bool original225;
    int icon_count;
    struct { int x,z,sprite; } icons[1000];
    int flag_scene_tile_x, flag_scene_tile_z; // x == 0: no destination (revision225)
    Pix32 *image_minimap; // 512x512 tile pixel buffer
    int32_t compass_mask_offsets[33];
    int32_t compass_mask_lengths[33];
    int32_t minimap_mask_offsets[151];
    int32_t minimap_mask_lengths[151];
} Minimap;

typedef struct { int click_x, click_y, tile_x, tile_z; } MinimapClick;
// Original handleMinimapInput. yaw is orbitCameraYaw + minimapAnticheatAngle, zoom minimapZoom.
bool minimap_click_target(int mouse_x, int mouse_y, int player_x, int player_z,
    int yaw, int zoom, MinimapClick *click);

void minimap_init(Minimap *m, Media *media, Arena *arena);
void minimap_draw_tile(uint32_t *pixels,int offset,int stride,const GroundTile *tile);
void minimap_build(Minimap *m, World *world, World3D *scene, Media *media, int level);
bool minimap_build225(Minimap *m,World *world,World3D *scene,Media *media,int level,
    LocConfig *locations,const CollisionMap *collision);
void minimap_draw(
    Minimap *m,
    Media *media,
    int player_x,
    int player_z,
    int camera_yaw,       // orbitCameraYaw (compass)
    int anticheat_angle,  // minimapAnticheatAngle, added for the map
    int zoom,             // minimapZoom
    int mapback_x,
    int mapback_y
);
void minimap_draw_dot(
    int player_x,
    int player_z,
    int target_x,
    int target_z,
    int yaw,              // orbitCameraYaw + minimapAnticheatAngle
    int zoom,
    int mapback_x,
    int mapback_y,
    Pix32 *dot_sprite,
    const Pix8 *mask
);
void minimap_free(Minimap *m);
void minimap_draw_npc_dot(int player_x, int player_z, const NpcEntity *npc,
    int yaw, int zoom, int mapback_x, int mapback_y, Pix32 *dot_sprite, const Pix8 *mask);
void minimap_draw_player_dot(int player_x, int player_z, const PlayerEntity *other,
    int yaw, int zoom, int mapback_x, int mapback_y, Pix32 *dot_sprite, const Pix8 *mask);
void minimap_draw_destination(Minimap *m, Media *media, int player_x, int player_z,
    int yaw, int zoom, int mapback_x, int mapback_y);

#endif // RS2_MINIMAP_H
