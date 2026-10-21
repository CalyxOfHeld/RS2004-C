#include "dash3d/minimap.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void minimap_init(Minimap *m, Media *media, Arena *arena) {
    if (!m) return;
    memset(m, 0, sizeof(Minimap));

    m->image_minimap = pix32_create(512, 512, arena);

    if (media && media->mapback) {
        Pix8 *mb = media->mapback;

        // Compass mask: lines 0..32 (33 lines)
        for (int y = 0; y < 33; y++) {
            int start_x = 999;
            int end_x = 0;
            for (int x = 0; x < 35; x++) {
                if (mb->pixels[x + y * mb->width] == 0) {
                    if (start_x == 999) start_x = x;
                } else if (start_x != 999) {
                    end_x = x;
                    break;
                }
            }
            m->compass_mask_offsets[y] = start_x;
            m->compass_mask_lengths[y] = end_x - start_x;
        }

        // Minimap mask: lines 9..159 (151 lines)
        for (int y = 9; y < 160; y++) {
            int start_x = 999;
            int end_x = 0;
            for (int x = 10; x < 168; x++) {
                if (mb->pixels[x + y * mb->width] == 0 && (x > 34 || y > 34)) {
                    if (start_x == 999) start_x = x;
                } else if (start_x != 999) {
                    end_x = x;
                    break;
                }
            }
            m->minimap_mask_offsets[y - 9] = start_x - 21;
            m->minimap_mask_lengths[y - 9] = end_x - start_x;
        }
    }
}

static const int MINIMAP_SHAPE[13][16] = { {0}, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1 }, { 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1 }, { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1 }, { 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1 } };
static const int MINIMAP_ROTATION[4][16] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, { 12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3 }, { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }, { 3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12 } };

// Bundled World3D.drawMinimapTile: masks are scene-shape and rotation indexed.
void minimap_draw_tile(uint32_t *pixels,int offset,int stride,const GroundTile *tile) {
    if(tile->has_underlay) {
        int rgb=tile->underlay.rgb;
        if(rgb) for(int y=0;y<4;y++) for(int x=0;x<4;x++) pixels[offset+y*stride+x]=rgb;
    } else if(tile->has_overlay) {
        const TileOverlay *o=&tile->overlay;
        if(o->shape<0 || o->shape>=13) return;
        for(int i=0;i<16;i++) {
            bool fg=MINIMAP_SHAPE[o->shape][MINIMAP_ROTATION[o->rotation&3][i]]!=0;
            if(fg || o->background_rgb) pixels[offset+(i/4)*stride+i%4]=fg ? o->foreground_rgb:o->background_rgb;
        }
    }
}
static void minimap_sprite(uint32_t *pixels,Pix8 *sprite,int x,int y) {
    if(!sprite) return;
    x+=sprite->crop_x;y+=sprite->crop_y;
    for(int sy=0;sy<sprite->height;sy++) for(int sx=0;sx<sprite->width;sx++) {
        int xx=x+sx,yy=y+sy,index=sprite->pixels[sy*sprite->width+sx];
        if(index && xx>=0 && xx<512 && yy>=0 && yy<512) pixels[yy*512+xx]=sprite->palette[index];
    }
}
static void minimap_wall(uint32_t *pixels,int off,int rotation,uint32_t rgb) {
    for(int i=0;i<4;i++) {
        int dx=rotation==0 ? 0:rotation==2 ? 3:i;
        int dy=rotation==1 ? 0:rotation==3 ? 3:i;
        pixels[off+dy*512+dx]=rgb;
    }
}
static void minimap_location(uint32_t *pixels, const WorldMapLocation *loc,Media *media) {
    int shape=loc->shape,r=loc->rotation;
    if(shape>=4 && shape<=8) return; // wall decoration has no map mark
    if(loc->mapscene>=0 && loc->mapscene<50) {
        Pix8 *sprite=media ? media->mapscene[loc->mapscene]:NULL;
        if(sprite) minimap_sprite(pixels,sprite,loc->x*4+48+(loc->width*4-sprite->width)/2,
            (104-loc->z-loc->length)*4+48+(loc->length*4-sprite->height)/2);
        return;
    }
    int off=loc->x*4+24624+(103-loc->z)*512*4;
    // Neutral value of original client's 238 +/-10 wall colour range.
    uint32_t rgb=loc->active ? 0xee0000:0xeeeeee;
    if(shape==0 || shape==2) minimap_wall(pixels,off,r,rgb);
    if(shape==2) minimap_wall(pixels,off,(r+1)&3,rgb);
    if(shape==3) {
        static const int corner[4]={0,3,1539,1536};pixels[off+corner[r]]=rgb;
    }
    if(shape==9) {
        rgb=loc->active ? 0xee0000:0xeeeeee;
        for(int i=0;i<4;i++) pixels[off+i*512+((r&1) ? i:3-i)]=rgb;
    }
}
void minimap_build(Minimap *m, World *world, World3D *scene, Media *media, int level) {
    if (!m || !m->image_minimap || !world || !scene || level<0 || level>=WORLD_LEVELS) return;
    m->scene=scene;m->level=level;
    uint32_t *pixels=m->image_minimap->pixels;
    memset(pixels,0,512*512*sizeof(uint32_t));
    for(int z=1;z<103;z++) for(int x=1;x<103;x++) {
        int offset=(103-z)*512*4+24628+(x-1)*4;
        if(!(world->tile_flags[level][x][z]&0x18))
            minimap_draw_tile(pixels,offset,512,&scene->tiles[level][x][z]);
        if(level<3 && (world->tile_flags[level+1][x][z]&8))
            minimap_draw_tile(pixels,offset,512,&scene->tiles[level+1][x][z]);
    }
    for(size_t i=0;i<scene->map_location_count;i++) {
        const WorldMapLocation *loc=&scene->map_locations[i];
        bool visible=loc->level==level && !(world->tile_flags[level][loc->x][loc->z]&0x18);
        if(level<3 && loc->level==level+1 && (world->tile_flags[level+1][loc->x][loc->z]&8)) visible=true;
        if(visible) minimap_location(pixels,loc,media);
    }
}

void minimap_draw(
    Minimap *m,
    Media *media,
    int player_x,
    int player_z,
    int camera_yaw,
    int mapback_x,
    int mapback_y
) {
    if (!m || !media) return;

    int cx = player_x / 32 + 48;
    int cy = 464 - player_z / 32;

    // Draw rotated minimap terrain
    if (m->image_minimap) {
        pix32_draw_rotated_masked(
            m->image_minimap,
            camera_yaw,
            146,
            m->minimap_mask_offsets,
            151,
            cy,
            256,
            cx,
            mapback_x + 21,
            mapback_y + 9,
            m->minimap_mask_lengths
        );
    }

    // Draw rotated compass needle
    if (media->compass) {
        pix32_draw_rotated_masked(
            media->compass,
            camera_yaw,
            33,
            m->compass_mask_offsets,
            33,
            25,
            256,
            25,
            mapback_x,
            mapback_y,
            m->compass_mask_lengths
        );
    }

    // Real ground-decoration mapfunction sprites. Keep exact cache tile
    // positions here; reference's collision-constrained random jitter is pending.
    if(m->scene) for(size_t i=0;i<m->scene->map_location_count;i++) {
        const WorldMapLocation *loc=&m->scene->map_locations[i];
        if(loc->level!=m->level || loc->shape!=22 || loc->mapfunction<0 || loc->mapfunction>=50) continue;
        Pix32 *sprite=media->mapfunction[loc->mapfunction];if(!sprite) continue;
        int dx=loc->x*4+2-player_x/32,dz=loc->z*4+2-player_z/32;
        if(dx*dx+dz*dz>6400) continue;
        int angle=camera_yaw&2047,sin=pix3d_sin_table[angle],cos=pix3d_cos_table[angle];
        int rx=(dz*sin+dx*cos)>>16,rz=(dz*cos-dx*sin)>>16;
        int left=94+rx-sprite->crop_w/2+sprite->crop_x;
        int top=83-rz-sprite->crop_h/2+sprite->crop_y;
        for(int y=0;y<sprite->height;y++) for(int x=0;x<sprite->width;x++) {
            int mx=left+x,my=top+y,xx=mapback_x+mx,yy=mapback_y+my;
            uint32_t rgb=sprite->pixels[y*sprite->width+x];
            if(!rgb || !media->mapback || mx<0 || my<0 || mx>=media->mapback->width || my>=media->mapback->height) continue;
            if(media->mapback->pixels[my*media->mapback->width+mx] || (mx<=34 && my<=34)) continue;
            if(xx>=pix2d_clip_left && xx<pix2d_clip_right && yy>=pix2d_clip_top && yy<pix2d_clip_bottom)
                pix2d_pixels[yy*pix2d_width+xx]=rgb;
        }
    }

    // Draw local player center white dot (3x3 pixels at center)
    pix2d_fill_rect(mapback_x + 93, mapback_y + 82, 3, 3, 0xFFFFFF);
}

void minimap_draw_dot(
    int player_x,
    int player_z,
    int target_x,
    int target_z,
    int camera_yaw,
    int mapback_x,
    int mapback_y,
    Pix32 *dot_sprite
) {
    if (!dot_sprite) return;

    // Revision-225 drawMinimap quantizes each position before subtraction.
    int dx = target_x / 32 - player_x / 32;
    int dz = target_z / 32 - player_z / 32;

    int dist_sq = dx * dx + dz * dz;
    if (dist_sq > 6400) return; // Outside minimap circle

    int angle = camera_yaw & 2047;
    int sin_val = pix3d_sin_table[angle];
    int cos_val = pix3d_cos_table[angle];

    int rx = (dz * sin_val + dx * cos_val) >> 16;
    int rz = (dz * cos_val - dx * sin_val) >> 16;

    int screen_x = mapback_x + 94 + rx - dot_sprite->crop_w / 2;
    int screen_y = mapback_y + 83 - rz - dot_sprite->crop_h / 2;

    pix32_draw(dot_sprite, screen_x, screen_y);
}

void minimap_draw_npc_dot(int player_x, int player_z, const NpcEntity *npc,
    int camera_yaw, int mapback_x, int mapback_y, Pix32 *dot_sprite) {
    // Revision-225 client.drawMinimap: isVisible(false) means type != null.
    if (!npc || !npc->has_type || !npc->type.minimap) return;
    minimap_draw_dot(player_x, player_z, npc->base.x, npc->base.z,
        camera_yaw, mapback_x, mapback_y, dot_sprite);
}

void minimap_draw_player_dot(int player_x, int player_z, const PlayerEntity *other,
    int camera_yaw, int mapback_x, int mapback_y, Pix32 *dot_sprite) {
    // Revision-225 PlayerEntity.isVisible and client.drawMinimap.
    if (!other || !other->visible) return;
    minimap_draw_dot(player_x, player_z, other->base.x, other->base.z,
        camera_yaw, mapback_x, mapback_y, dot_sprite);
}

void minimap_draw_destination(Minimap *m, Media *media, int player_x, int player_z,
    int camera_yaw, int mapback_x, int mapback_y) {
    if (!m || !media || !media->mapflag || m->flag_scene_tile_x == 0) return;
    Pix32 *sprite = media->mapflag;
    int dx = m->flag_scene_tile_x * 4 + 2 - player_x / 32;
    int dz = m->flag_scene_tile_z * 4 + 2 - player_z / 32;
    int distance = dx * dx + dz * dz;
    if (distance > 6400) return;
    int angle = camera_yaw & 2047;
    int rx = (dz * pix3d_sin_table[angle] + dx * pix3d_cos_table[angle]) >> 16;
    int rz = (dz * pix3d_cos_table[angle] - dx * pix3d_sin_table[angle]) >> 16;
    int left = 94 + rx - sprite->crop_w / 2;
    int top = 83 - rz - sprite->crop_h / 2;
    if (distance <= 2500) {
        pix32_draw(sprite, mapback_x + left, mapback_y + top);
        return;
    }
    // Original drawOnMinimap masks distant markers against imageMapback.
    if (!media->mapback) return;
    for (int y = 0; y < sprite->height; y++) for (int x = 0; x < sprite->width; x++) {
        int mx = left + sprite->crop_x + x, my = top + sprite->crop_y + y;
        int sx = mapback_x + mx, sy = mapback_y + my;
        uint32_t rgb = sprite->pixels[y * sprite->width + x];
        if (!rgb || mx < 0 || my < 0 || mx >= media->mapback->width || my >= media->mapback->height) continue;
        if (media->mapback->pixels[my * media->mapback->width + mx]) continue;
        if (sx >= pix2d_clip_left && sx < pix2d_clip_right && sy >= pix2d_clip_top && sy < pix2d_clip_bottom)
            pix2d_pixels[sy * pix2d_width + sx] = rgb;
    }
}

void minimap_free(Minimap *m) {
    if (m && m->image_minimap) {
        pix32_free(m->image_minimap);
        m->image_minimap = NULL;
    }
}
