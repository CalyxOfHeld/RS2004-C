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

typedef struct {
    const World3D *scene;
    int level;
    Pix32 *image_minimap; // 512x512 tile pixel buffer
    int32_t compass_mask_offsets[33];
    int32_t compass_mask_lengths[33];
    int32_t minimap_mask_offsets[151];
    int32_t minimap_mask_lengths[151];
} Minimap;

void minimap_init(Minimap *m, Media *media, Arena *arena);
void minimap_draw_tile(uint32_t *pixels,int offset,int stride,const GroundTile *tile);
void minimap_build(Minimap *m, World *world, World3D *scene, Media *media, int level);
void minimap_draw(
    Minimap *m,
    Media *media,
    int player_x,
    int player_z,
    int camera_yaw,
    int mapback_x,
    int mapback_y
);
void minimap_draw_dot(
    int player_x,
    int player_z,
    int target_x,
    int target_z,
    int camera_yaw,
    int mapback_x,
    int mapback_y,
    Pix32 *dot_sprite
);
void minimap_free(Minimap *m);
void minimap_draw_npc_dot(int player_x, int player_z, const NpcEntity *npc,
    int camera_yaw, int mapback_x, int mapback_y, Pix32 *dot_sprite);

#endif // RS2_MINIMAP_H
