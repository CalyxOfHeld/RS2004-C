#include "dash3d/minimap.h"
#include "gfx/pix2d.h"
#include "core/jrandom.h"
#include "dash3d/scene_build225.h"
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
static void minimap_location(uint32_t *pixels, const WorldMapLocation *loc,Media *media,uint32_t white,uint32_t red) {
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
    uint32_t rgb=loc->active ? red:white;
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
    m->original225=false;
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
        if(visible) minimap_location(pixels,loc,media,0xeeeeee,0xee0000);
    }
}

/* Reuse the qualified original scene graph for bridge shifts, refused spans
 * and wall/location/ground-decoration membership. This temporary adapter owns
 * no native models and is released before returning. */
static SceneGround225 *minimap_ground225(SceneTraversal225 *s,int level,int x,int z) {
    int id=s->grid[(level*104+x)*104+z];
    return id<0 ? NULL:&s->grounds[id];
}
static const WorldMapLocation *minimap_meta225(const World3D *native,int level,int x,int z,int bitset,int bucket) {
    const WorldMapLocation *found=NULL;
    for(size_t i=0;i<native->map_location_count;i++) {
        const WorldMapLocation *loc=&native->map_locations[i];
        if(loc->level!=level || loc->x!=x || loc->z!=z || loc->id!=((bitset>>14)&32767) || loc->active!=(bitset>0))continue;
        if((bucket==0 && loc->shape<4) || (bucket==1 && loc->shape>=9 && loc->shape!=22) ||
            (bucket==2 && loc->shape==22) || (bucket==3 && loc->shape>=4 && loc->shape<=8)){
            found=loc;if(bucket==1)break;
        }
    }
    return found;
}
static void minimap_locs225(Minimap *m,World3D *native,SceneTraversal225 *s,Media *media,
    int level,int x,int z,uint32_t white,uint32_t red) {
    SceneGround225 *g=minimap_ground225(s,level,x,z);if(!g)return;
    int bits[3]={g->wall.present?g->wall.bitset:0,0,g->ground_decor.present?g->ground_decor.bitset:0};
    for(int i=0;i<g->loc_count;i++) {
        SceneLocation225 *loc=&s->locations[g->locs[i]];
        if(((loc->bitset>>29)&3)==2 && loc->min_x==x && loc->min_z==z){bits[1]=loc->bitset;break;}
    }
    for(int bucket=0;bucket<3;bucket++)if(bits[bucket]) {
        const WorldMapLocation *loc=minimap_meta225(native,g->occlude_level,x,z,bits[bucket],bucket);
        if(loc) {
            WorldMapLocation mark=*loc;
            /* World3D.getInfo searches wall, wall decoration, ground
             * decoration, then locations by bitset. Different shapes of the
             * same loc ID at one tile share that bitset in the original. */
            int info_bucket=bucket;
            if(g->wall.present && g->wall.bitset==bits[bucket])info_bucket=0;
            else if(g->decor.present && g->decor.bitset==bits[bucket])info_bucket=3;
            else if(g->ground_decor.present && g->ground_decor.bitset==bits[bucket])info_bucket=2;
            const WorldMapLocation *info=minimap_meta225(native,g->occlude_level,x,z,bits[bucket],info_bucket);
            if(info){mark.shape=info->shape;mark.rotation=info->rotation;}
            if(bucket==1 && mark.mapscene<0 && mark.shape!=9)continue;
            minimap_location(m->image_minimap->pixels,&mark,media,white,red);
        }
    }
}
bool minimap_build225(Minimap *m,World *world,World3D *native,Media *media,int level,
    LocConfig *locations,const CollisionMap *collision) {
    if(!m || !m->image_minimap || !world || !native || !locations || !collision || level<0 || level>3)return false;
    SceneBuild225 *build=world3d_construct225(world,native,locations);if(!build)return false;
    SceneTraversal225 *s=world3d_scene225(build);
    const SceneGeometry225 *geometry=world3d_scene_geometry225(build);
    m->scene=native;m->level=level;m->original225=true;m->icon_count=0;
    uint32_t *pixels=m->image_minimap->pixels;memset(pixels,0,512*512*sizeof(*pixels));
    for(int z=1;z<103;z++)for(int x=1;x<103;x++)for(int above=0;above<2;above++) {
        int l=level+above;if(l>3)continue;
        if(above ? !(world->tile_flags[l][x][z]&8):(world->tile_flags[l][x][z]&0x18))continue;
        SceneGround225 *g=minimap_ground225(s,l,x,z);if(!g)continue;
        GroundTile tile={0};
        if(geometry[g->id].underlay){tile.has_underlay=true;tile.underlay=*geometry[g->id].underlay;}
        if(geometry[g->id].overlay){tile.has_overlay=true;tile.overlay=*geometry[g->id].overlay;}
        minimap_draw_tile(pixels,(103-z)*512*4+24624+x*4,512,&tile);
    }
    /* Keep the four Math.random calls in source order. */
    uint32_t white=(uint32_t)((int)(math_random()*20)+228)<<16;
    white|=(uint32_t)((int)(math_random()*20)+228)<<8;
    white|=(uint32_t)((int)(math_random()*20)+228);
    uint32_t red=(uint32_t)((int)(math_random()*20)+228)<<16;
    for(int z=1;z<103;z++)for(int x=1;x<103;x++) {
        if(!(world->tile_flags[level][x][z]&0x18))minimap_locs225(m,native,s,media,level,x,z,white,red);
        if(level<3 && (world->tile_flags[level+1][x][z]&8))minimap_locs225(m,native,s,media,level+1,x,z,white,red);
    }
    for(int x=0;x<104;x++)for(int z=0;z<104;z++) {
        SceneGround225 *g=minimap_ground225(s,level,x,z);if(!g || !g->ground_decor.present)continue;
        const WorldMapLocation *loc=minimap_meta225(native,g->occlude_level,x,z,g->ground_decor.bitset,2);
        if(!loc || loc->mapfunction<0)continue;
        int icon=loc->mapfunction,px=x,pz=z;
        if(icon!=22 && icon!=29 && icon!=34 && icon!=36 && icon!=46 && icon!=47 && icon!=48) {
            for(int i=0;i<10;i++) {
                int direction=(int)(math_random()*4);
                if(direction==0 && px>0 && px>x-3 && !(collision[level].flags[px-1][pz]&0x280108))px--;
                if(direction==1 && px<103 && px<x+3 && !(collision[level].flags[px+1][pz]&0x280180))px++;
                if(direction==2 && pz>0 && pz>z-3 && !(collision[level].flags[px][pz-1]&0x280102))pz--;
                if(direction==3 && pz<103 && pz<z+3 && !(collision[level].flags[px][pz+1]&0x280120))pz++;
            }
        }
        if(m->icon_count>=1000){world3d_destroy_scene225(build);return false;}
        m->icons[m->icon_count].x=px;m->icons[m->icon_count].z=pz;m->icons[m->icon_count++].sprite=icon;
    }
    world3d_destroy_scene225(build);return true;
}

void minimap_draw(
    Minimap *m,
    Media *media,
    int player_x,
    int player_z,
    int camera_yaw,
    int anticheat_angle,
    int zoom,
    int mapback_x,
    int mapback_y
) {
    if (!m || !media) return;
    int map_yaw = (camera_yaw + anticheat_angle) & 0x7FF;   // client.java:3494

    int cx = player_x / 32 + 48;
    int cy = 464 - player_z / 32;

    // Draw rotated minimap terrain
    if (m->image_minimap) {
        pix32_draw_rotated_masked(
            m->image_minimap,
            map_yaw,
            146,
            m->minimap_mask_offsets,
            151,
            cy,
            zoom + 256,
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

    // Original builds cache collision-constrained positions until rebuild.
    // Legacy builds retain their exact cache tile positions.
    if(m->original225) for(int i=0;i<m->icon_count;i++) {
        int icon=m->icons[i].sprite;if(icon<0 || icon>=50)continue;
        minimap_draw_dot(player_x,player_z,m->icons[i].x*128+64,m->icons[i].z*128+64,
            map_yaw,zoom,mapback_x,mapback_y,media->mapfunction[icon],media->mapback);
    }
    else if(m->scene) for(size_t i=0;i<m->scene->map_location_count;i++) {
        const WorldMapLocation *loc=&m->scene->map_locations[i];
        if(loc->level!=m->level || loc->shape!=22 || loc->mapfunction<0 || loc->mapfunction>=50) continue;
        Pix32 *sprite=media->mapfunction[loc->mapfunction];if(!sprite) continue;
        minimap_draw_dot(player_x,player_z,loc->x*128+64,loc->z*128+64,
            map_yaw,zoom,mapback_x,mapback_y,sprite,media->mapback);
    }

    // Draw local player center white dot (3x3 pixels at center)
    pix2d_fill_rect(mapback_x + 93, mapback_y + 82, 3, 3, 0xFFFFFF);
}

void minimap_draw_dot(
    int player_x,
    int player_z,
    int target_x,
    int target_z,
    int yaw,
    int zoom,
    int mapback_x,
    int mapback_y,
    Pix32 *dot_sprite,
    const Pix8 *mask
) {
    if (!dot_sprite) return;

    // Revision-225 drawMinimap quantizes each position before subtraction.
    int dx = target_x / 32 - player_x / 32;
    int dz = target_z / 32 - player_z / 32;

    int dist_sq = dx * dx + dz * dz;
    if (dist_sq > 6400) return; // Outside minimap circle

    // client.drawOnMinimap (client.java:6770-6782).
    int angle = yaw & 2047;
    int sin_val = pix3d_sin_table[angle] * 256 / (zoom + 256);
    int cos_val = pix3d_cos_table[angle] * 256 / (zoom + 256);

    int rx = (dz * sin_val + dx * cos_val) >> 16;
    int rz = (dz * cos_val - dx * sin_val) >> 16;

    int screen_x = mapback_x + 94 + rx - dot_sprite->crop_w / 2;
    int screen_y = mapback_y + 83 - rz - dot_sprite->crop_h / 2;

    if (dist_sq <= 2500) {
        pix32_draw(dot_sprite, screen_x, screen_y);
        return;
    }
    // Original drawOnMinimap uses Pix32.drawMasked beyond distance 50.
    if (!mask) return;
    for (int y = 0; y < dot_sprite->height; y++) for (int x = 0; x < dot_sprite->width; x++) {
        int sx = screen_x + dot_sprite->crop_x + x, sy = screen_y + dot_sprite->crop_y + y;
        int mx = sx - mapback_x, my = sy - mapback_y;
        uint32_t rgb = dot_sprite->pixels[y * dot_sprite->width + x];
        if (!rgb || mx < 0 || my < 0 || mx >= mask->width || my >= mask->height) continue;
        if (mask->pixels[my * mask->width + mx]) continue;
        if (sx >= pix2d_clip_left && sx < pix2d_clip_right && sy >= pix2d_clip_top && sy < pix2d_clip_bottom)
            pix2d_pixels[sy * pix2d_width + sx] = rgb;
    }
}

void minimap_draw_npc_dot(int player_x, int player_z, const NpcEntity *npc,
    int yaw, int zoom, int mapback_x, int mapback_y, Pix32 *dot_sprite, const Pix8 *mask) {
    // Revision-225 client.drawMinimap: isVisible(false) means type != null.
    if (!npc || !npc->has_type || !npc->type.minimap) return;
    minimap_draw_dot(player_x, player_z, npc->base.x, npc->base.z,
        yaw, zoom, mapback_x, mapback_y, dot_sprite, mask);
}

void minimap_draw_player_dot(int player_x, int player_z, const PlayerEntity *other,
    int yaw, int zoom, int mapback_x, int mapback_y, Pix32 *dot_sprite, const Pix8 *mask) {
    // Revision-225 PlayerEntity.isVisible and client.drawMinimap.
    if (!other || !other->visible) return;
    minimap_draw_dot(player_x, player_z, other->base.x, other->base.z,
        yaw, zoom, mapback_x, mapback_y, dot_sprite, mask);
}

void minimap_draw_destination(Minimap *m, Media *media, int player_x, int player_z,
    int yaw, int zoom, int mapback_x, int mapback_y) {
    if (!m || !media || !media->mapflag || m->flag_scene_tile_x == 0) return;
    minimap_draw_dot(player_x,player_z,m->flag_scene_tile_x*128+64,m->flag_scene_tile_z*128+64,
        yaw,zoom,mapback_x,mapback_y,media->mapflag,media->mapback);
}

bool minimap_click_target(int mouse_x, int mouse_y, int player_x, int player_z,
    int yaw_in, int zoom, MinimapClick *click) {
    if (!click || mouse_x < 582 || mouse_x >= 728 || mouse_y < 14 || mouse_y >= 165) return false;
    int x = mouse_x - 582 - 73, y = mouse_y - 14 - 75;
    int yaw = yaw_in & 2047;
    int sin = pix3d_sin_table[yaw] * (zoom + 256) >> 8, cos = pix3d_cos_table[yaw] * (zoom + 256) >> 8;
    int dx = (y * sin + x * cos) >> 11;
    int dz = (y * cos - x * sin) >> 11;
    *click = (MinimapClick){x, y, (player_x + dx) >> 7, (player_z - dz) >> 7};
    return true;
}

void minimap_free(Minimap *m) {
    if (m && m->image_minimap) {
        pix32_free(m->image_minimap);
        m->image_minimap = NULL;
    }
}
