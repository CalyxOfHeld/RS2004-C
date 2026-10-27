#include "dash3d/world.h"
#include "dash3d/location_anim.h"
#include "gfx/pix3d.h"
#include "core/packet.h"
#include "core/jrandom.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int noise(int x, int z) {
    int n = x + z * 57;
    n = (n << 13) ^ n;
    int res = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7FFFFFFF;
    return (res >> 19) & 0xFF;
}

static int smooth_noise(int x, int z) {
    int corners = noise(x - 1, z - 1) + noise(x + 1, z - 1) + noise(x - 1, z + 1) + noise(x + 1, z + 1);
    int sides = noise(x - 1, z) + noise(x + 1, z) + noise(x, z - 1) + noise(x, z + 1);
    int center = noise(x, z);
    return corners / 16 + sides / 8 + center / 4;
}

static int interpolate(int a, int b, int x, int scale) {
    int f = (65536 - pix3d_cos_table[(x * 1024 / scale) & 2047]) >> 1;
    return ((a * (65536 - f)) >> 16) + ((b * f) >> 16);
}

static int interpolated_noise(int x, int z, int scale) {
    int int_x = x / scale;
    int frac_x = x & (scale - 1);
    int int_z = z / scale;
    int frac_z = z & (scale - 1);
    int v1 = smooth_noise(int_x, int_z);
    int v2 = smooth_noise(int_x + 1, int_z);
    int v3 = smooth_noise(int_x, int_z + 1);
    int v4 = smooth_noise(int_x + 1, int_z + 1);
    int i1 = interpolate(v1, v2, frac_x, scale);
    int i2 = interpolate(v3, v4, frac_x, scale);
    return interpolate(i1, i2, frac_z, scale);
}

int world_perlin_noise(int x, int z) {
    int v = interpolated_noise(x + 45365, z + 91923, 4) - 128
          + ((interpolated_noise(x + 10294, z + 37821, 2) - 128) >> 1)
          + ((interpolated_noise(x, z, 1) - 128) >> 2);
    v = (int)((double)v * 0.3) + 35;
    if (v < 10) v = 10;
    else if (v > 60) v = 60;
    return v;
}

// ClientBuild.getUCol / getOCol: absent underlay and transparent overlay
// are distinct from a textured overlay (whose shade is 127-light).
static int underlay_color(int hsl, int light) {
    if (hsl == -1) return 12345678;
    light = light * (hsl & 127) / 128;
    if (light < 2) light = 2;
    if (light > 126) light = 126;
    return (hsl & 0xff80) + light;
}
static int overlay_color(int hsl, int light) {
    if (hsl == -2) return 12345678;
    if (hsl == -1) {
        if (light < 0) light = 0;
        if (light > 127) light = 127;
        return 127-light;
    }
    return underlay_color(hsl, light);
}

void world_init(World *w, int max_x, int max_z) {
    memset(w, 0, sizeof(World));
    w->max_tile_x = max_x > WORLD_SIZE_X ? WORLD_SIZE_X : max_x;
    w->max_tile_z = max_z > WORLD_SIZE_Z ? WORLD_SIZE_Z : max_z;
    world_reset(w);
}

void world_reset(World *w) {
    memset(w->heightmap, 0, sizeof(w->heightmap));
    memset(w->tile_flags, 0, sizeof(w->tile_flags));
    memset(w->tile_underlays, 0, sizeof(w->tile_underlays));
    memset(w->tile_overlays, 0, sizeof(w->tile_overlays));
    memset(w->tile_overlay_shapes, 0, sizeof(w->tile_overlay_shapes));
    memset(w->tile_overlay_rotations, 0, sizeof(w->tile_overlay_rotations));
    memset(w->shademap, 0, sizeof(w->shademap));
    memset(w->occludemap,0,sizeof(w->occludemap));
}

void world_load_ground(World *w, const uint8_t *data, size_t len, int x_offset, int z_offset, int base_x, int base_z) {
    Packet pkt = packet_wrap((uint8_t*)data, len);

    for (int level = 0; level < 4; level++) {
        for (int x = 0; x < 64; x++) {
            for (int z = 0; z < 64; z++) {
                int tile_x = x + x_offset;
                int tile_z = z + z_offset;

                if (tile_x >= 0 && tile_x < w->max_tile_x && tile_z >= 0 && tile_z < w->max_tile_z) {
                    w->tile_flags[level][tile_x][tile_z] = 0;

                    while (1) {
                        int code = g1(&pkt);
                        if (code == 0) {
                            if (level == 0) {
                                w->heightmap[0][tile_x][tile_z] = -world_perlin_noise(tile_x + 932731 + base_x, tile_z + 556238 + base_z) * 8;
                            } else {
                                w->heightmap[level][tile_x][tile_z] = w->heightmap[level - 1][tile_x][tile_z] - 240;
                            }
                            break;
                        }

                        if (code == 1) {
                            int height_val = g1(&pkt);
                            if (height_val == 1) height_val = 0;
                            if (level == 0) {
                                w->heightmap[0][tile_x][tile_z] = -height_val * 8;
                            } else {
                                w->heightmap[level][tile_x][tile_z] = w->heightmap[level - 1][tile_x][tile_z] - height_val * 8;
                            }
                            break;
                        }

                        if (code <= 49) {
                            w->tile_overlays[level][tile_x][tile_z] = (uint8_t)g1b(&pkt);
                            w->tile_overlay_shapes[level][tile_x][tile_z] = (uint8_t)((code - 2) / 4);
                            w->tile_overlay_rotations[level][tile_x][tile_z] = (uint8_t)((code - 2) & 3);
                        } else if (code <= 81) {
                            w->tile_flags[level][tile_x][tile_z] = (uint8_t)(code - 49);
                        } else {
                            w->tile_underlays[level][tile_x][tile_z] = (uint8_t)(code - 81);
                        }
                    }
                } else {
                    // Out of bounds tile skip
                    while (1) {
                        int code = g1(&pkt);
                        if (code == 0) break;
                        if (code == 1) { g1(&pkt); break; }
                        if (code <= 49) g1(&pkt);
                    }
                }
            }
        }
    }
}

static bool place_location(World *w,LocConfig *loc_cfg,World3D *world3d,CollisionMap *cm,int level,int tile_x,int tile_z,int loc_id,int shape,int rotation,bool shadows) {
    if(!loc_cfg || level<0 || level>=4 || tile_x<=0 || tile_z<=0 || tile_x>=w->max_tile_x-1 || tile_z>=w->max_tile_z-1) return false;
    int effective_level=level-((w->tile_flags[1][tile_x][tile_z]&2) ? 1:0);
    LocType loc;
    if (loc_get(loc_cfg, loc_id, &loc)) {
        if(w->build225){
            if(shape>=12 && shape<=17 && shape!=13 && level>0)
                w->occludemap[level][tile_x][tile_z]|=0x924;
            if(loc.occlude && (shape==0 || shape==2)){
                static const int dx[4]={0,0,1,0},dz[4]={0,1,0,0};
                int mask=(rotation&1)?0x492:0x249;
                w->occludemap[level][tile_x+dx[rotation]][tile_z+dz[rotation]]|=mask;
                if(shape==2){
                    int next=(rotation+1)&3;
                    w->occludemap[level][tile_x+dx[next]][tile_z+dz[next]]|=(next&1)?0x492:0x249;
                }
            }
        }
        if(world3d) world3d_add_map_location(world3d,(WorldMapLocation){level,tile_x,tile_z,loc_id,shape,rotation,
            loc.width,loc.length,loc.mapscene,loc.mapfunction,loc.active});
        if(shadows && loc.shadow && (shape==0 || shape==1 || shape==3)) {
            int xx=tile_x,zz=tile_z;
            if(shape==0) {
                static const int corners[4][4]={{0,0,0,1},{0,1,1,1},{1,0,1,1},{0,0,1,0}};
                w->shademap[level][xx+corners[rotation][0]][zz+corners[rotation][1]]=50;
                w->shademap[level][xx+corners[rotation][2]][zz+corners[rotation][3]]=50;
            } else {
                static const int corners[4][2]={{0,1},{1,1},{1,0},{0,0}};
                w->shademap[level][xx+corners[rotation][0]][zz+corners[rotation][1]]=50;
            }
        }

        int collision_width=(rotation&1) ? loc.length:loc.width;
        int collision_length=(rotation&1) ? loc.width:loc.length;
        if(cm && effective_level>=0 && loc.blockwalk) {
            if(shape<=3) collision_map_add_wall(&cm[effective_level],tile_x,tile_z,shape,rotation,loc.blockrange);
            else if(shape==22) {if(loc.active) collision_map_block_ground(&cm[effective_level],tile_x,tile_z,true);}
            else if(shape>=9) collision_map_add_loc(&cm[effective_level],tile_x,tile_z,collision_width,collision_length,loc.blockrange);
        }
        // Rendering stays on the original map level. LinkBelow
        // only changes collision plane (ClientBuild.addLoc).
        int y_sw = w->heightmap[level][tile_x][tile_z];
        int y_se = w->heightmap[level][tile_x + 1][tile_z];
        int y_ne = w->heightmap[level][tile_x + 1][tile_z + 1];
        int y_nw = w->heightmap[level][tile_x][tile_z + 1];
        int avg_y = (y_sw + y_se + y_ne + y_nw) >> 2;

        int w_len = loc.length;
        int w_wid = loc.width;
        if (rotation == 1 || rotation == 3) {
            w_len = loc.width;
            w_wid = loc.length;
        }

        int model_shape=shape==11 ? 10:(shape>=4 && shape<=8) ? 4:shape;
        bool decor=shape>=4 && shape<=8;
        Model *parts[2]={world3d ? loc_get_model(&loc,model_shape,decor ? 0:shape==2 ? rotation+4:rotation,NULL):NULL,NULL};
        if(world3d && shape==2) parts[1]=loc_get_model(&loc,2,(rotation+1)&3,NULL);
        for(int part=0;part<2;part++) {Model *m=parts[part];
        if (m) {
            // Static locations retain pre-skew normals for World3D.buildModels.
            // Dynamic replacement keeps the existing individual-light fallback.
            if(loc.sharelight && !shadows) model_calculate_normals(m,64+loc.ambient,768+loc.contrast*5,-50,-10,-50);
            loc_skew_model(m,&loc,y_sw,y_se,y_ne,y_nw);
            int bitset = 0xC0000000 | (loc_id << 14) | (tile_x << 7) | tile_z;
            int offset=16;
            if(world3d && shape==5) {
                GroundTile *tile=&world3d->tiles[level][tile_x][tile_z];
                for(int j=tile->loc_count-1;j>=0;j--) if(tile->locs[j].shape<4) {
                    LocType wall;if(loc_get(loc_cfg,(tile->locs[j].bitset>>14)&32767,&wall)) {
                        if(wall.active) offset=wall.wallwidth; // Original getWallBitset > 0.
                        loc_free_entry(&wall);
                    }
                    break;
                }
            }
            bool added=world3d_add_loc(world3d,level,tile_x,tile_z,avg_y,m,bitset,decor ? 1:w_wid,decor ? 1:w_len);
            if(added) {
                GroundTile *tile=&world3d->tiles[level][tile_x][tile_z];WorldLocation *placed=&tile->locs[tile->loc_count-1];
                placed->shape=shape;placed->rotation=rotation;placed->owns_model=true;
                placed->yaw=decor ? rotation*512:shape==11 ? 256:0;
                if(shape==5) {static const int dx[]={1,0,-1,0},dz[]={0,-1,0,1};placed->x+=dx[rotation]*offset;placed->z+=dz[rotation]*offset;}
                if(shadows && (shape==0 || shape==2) && loc.wallwidth!=16) for(int j=0;j<tile->loc_count;j++) {
                    WorldLocation *d=&tile->locs[j];if(d->shape<4 || d->shape>8) continue;
                    int cx=tile_x*128+64,cz=tile_z*128+64;
                    d->x=cx+(d->x-cx)*loc.wallwidth/16;d->z=cz+(d->z-cz)*loc.wallwidth/16;
                }
            }
            else model_free_unshared(m);
            if(shadows && added && loc.shadow && (shape==10 || shape==11)) {
                double radius2=0;
                for(int v=0;v<m->num_points;v++) {
                    double rr=(double)m->point_x[v]*m->point_x[v]+(double)m->point_z[v]*m->point_z[v];
                    if(rr>radius2) radius2=rr;
                }
                int shade=(int)(sqrt(radius2)+0.99)/4;if(shade>30) shade=30;
                for(int dx=0;dx<=w_wid && tile_x+dx<=w->max_tile_x;dx++)
                    for(int dz=0;dz<=w_len && tile_z+dz<=w->max_tile_z;dz++)
                        if(shade>w->shademap[level][tile_x+dx][tile_z+dz]) w->shademap[level][tile_x+dx][tile_z+dz]=(int8_t)shade;
            }
        }
        }
        if(world3d && world3d->sequences && loc.anim>=0)
            location_anim_add(world3d,loc_id,level,tile_x,tile_z,world_location_layer(shape),loc.anim,true);
        loc_free_entry(&loc);
        return true;
    }
    return false;
}

void world_load_locations(World *w, const uint8_t *data, size_t len, LocConfig *loc_cfg, World3D *world3d, CollisionMap *cm, int x_offset, int z_offset) {
    Packet pkt = packet_wrap((uint8_t*)data, len);

    int loc_id = -1;
    while (1) {
        int delta_id = gsmarts(&pkt);
        if (delta_id == 0) {
            return;
        }

        loc_id += delta_id;
        int packed_pos = 0;

        while (1) {
            int delta_pos = gsmarts(&pkt);
            if (delta_pos == 0) {
                break;
            }

            packed_pos += delta_pos - 1;
            int local_z = packed_pos & 0x3F;
            int local_x = (packed_pos >> 6) & 0x3F;
            int level = packed_pos >> 12;

            int info = g1(&pkt);
            int shape = info >> 2;
            int rotation = info & 3;

            int tile_x = local_x + x_offset;
            int tile_z = local_z + z_offset;

            if (level>=0 && level<WORLD_LEVELS && tile_x > 0 && tile_z > 0 && tile_x < w->max_tile_x - 1 && tile_z < w->max_tile_z - 1) {
                place_location(w,loc_cfg,world3d,cm,level,tile_x,tile_z,loc_id,shape,rotation,true);
            }
        }
    }
}

void world_build(World *w, World3D *world3d, FloConfig *flo_cfg, CollisionMap *cm) {
    world3d->build_revision++;
    if(w->build225){
        w->random_hue_offset+=(int)(math_random()*5.0)-2;
        if(w->random_hue_offset < -8)w->random_hue_offset=-8;
        if(w->random_hue_offset > 8)w->random_hue_offset=8;
        w->random_lightness_offset+=(int)(math_random()*5.0)-2;
        if(w->random_lightness_offset < -16)w->random_lightness_offset=-16;
        if(w->random_lightness_offset > 16)w->random_lightness_offset=16;
    }
    if(cm) for(int level=0;level<WORLD_LEVELS;level++) for(int x=0;x<w->max_tile_x;x++) for(int z=0;z<w->max_tile_z;z++) {
        if(!(w->tile_flags[level][x][z]&1)) continue;
        int collision_level=level-((w->tile_flags[1][x][z]&2) ? 1:0);
        if(collision_level>=0) collision_map_block_ground(&cm[collision_level],x,z,true);
    }
    memcpy(world3d->heightmap, w->heightmap, sizeof(w->heightmap));
    if(!world3d_build_models(world3d)) fprintf(stderr,"Unable to build shared scenery lighting\n");
    // Revision-225 World.getDrawLevel / World3D.draw: logical visibility is
    // independent of the source height/model plane (bridges and flag-8 tiles).
    for(int l=0;l<WORLD_LEVELS;l++) for(int x=0;x<w->max_tile_x;x++) for(int z=0;z<w->max_tile_z;z++)
        world3d->tiles[l][x][z].draw_level=(w->tile_flags[l][x][z]&8) ? 0:
            l>0 && (w->tile_flags[1][x][z]&2) ? l-1:l;
    if (!flo_cfg || !flo_cfg->instances) return;
    // Port of bundled ClientBuild.finishBuild: height normals/shadows and
    // sliding weighted underlay blend. C FloType.chroma is underlayHue;
    // FloType.luminance is the reference's chroma multiplier.
    int light[WORLD_SIZE_X+1][WORLD_SIZE_Z+1];
    int hue[WORLD_SIZE_Z],sat[WORLD_SIZE_Z],lig[WORLD_SIZE_Z];
    int chroma[WORLD_SIZE_Z],count[WORLD_SIZE_Z];
    const int magnitude = (768 * (int)sqrt(5100.0)) >> 8;
    for (int l=0;l<WORLD_LEVELS;l++) {
        memset(light,0,sizeof(light));
        for(int x=1;x<w->max_tile_x-1;x++) for(int z=1;z<w->max_tile_z-1;z++) {
            int dx=w->heightmap[l][x+1][z]-w->heightmap[l][x-1][z];
            int dz=w->heightmap[l][x][z+1]-w->heightmap[l][x][z-1];
            int len=(int)sqrt((double)dx*dx+65536.0+(double)dz*dz);
            int nx=dx*256/len,ny=65536/len,nz=dz*256/len;
            int shade=(w->shademap[l][x-1][z]>>2)+(w->shademap[l][x+1][z]>>3)
                +(w->shademap[l][x][z-1]>>2)+(w->shademap[l][x][z+1]>>3)
                +(w->shademap[l][x][z]>>1);
            light[x][z]=96+(-50*nx-10*ny-50*nz)/magnitude-shade;
        }
        memset(hue,0,sizeof(hue));memset(sat,0,sizeof(sat));memset(lig,0,sizeof(lig));
        memset(chroma,0,sizeof(chroma));memset(count,0,sizeof(count));
        for(int x=-5;x<w->max_tile_x+5;x++) {
            for(int z=0;z<w->max_tile_z;z++) for(int side=0;side<2;side++) {
                int xx=x+(side ? -5:5),sign=side ? -1:1;
                if(xx<0 || xx>=w->max_tile_x) continue;
                int id=w->tile_underlays[l][xx][z];
                if(id<1 || id>flo_cfg->count) continue;
                FloType *f=&flo_cfg->instances[id-1];
                hue[z]+=sign*f->chroma;sat[z]+=sign*f->saturation;lig[z]+=sign*f->lightness;
                chroma[z]+=sign*f->luminance;count[z]+=sign;
            }
            if(x<1 || x>=w->max_tile_x-1) continue;
            int bh=0,bs=0,bl=0,bc=0,bt=0;
            for(int z=-5;z<w->max_tile_z+5;z++) {
                for(int side=0;side<2;side++) {
                    int zz=z+(side ? -5:5),sign=side ? -1:1;
                    if(zz<0 || zz>=w->max_tile_z) continue;
                    bh+=sign*hue[zz];bs+=sign*sat[zz];bl+=sign*lig[zz];
                    bc+=sign*chroma[zz];bt+=sign*count[zz];
                }
                if(z<1 || z>=w->max_tile_z-1) continue;
                int under=w->tile_underlays[l][x][z],over=w->tile_overlays[l][x][z];
                if(!under && !over) continue;
                int base=-1,bg=0;
                if(under>0 && under<=flo_cfg->count && bc>0 && bt>0) {
                    base=flo_hsl24to16(bh*256/bc,bs/bt,bl/bt);
                    // Neutral session hue/light offsets: both within the original
                    // client's random ranges; no per-tile fabricated variation.
                    int display=base;
                    if(w->build225){
                        int lightness=bl/bt+w->random_lightness_offset;
                        if(lightness<0)lightness=0;else if(lightness>255)lightness=255;
                        display=flo_hsl24to16((bh*256/bc+w->random_hue_offset)&255,bs/bt,lightness);
                    }
                    bg=pix3d_colour_table[underlay_color(display,96)];
                }
                if(w->build225 && l>0){
                    bool occlude=!(under==0 && w->tile_overlay_shapes[l][x][z]!=0);
                    if(over>0 && over<=flo_cfg->count && !flo_cfg->instances[over-1].occlude)occlude=false;
                    int h=w->heightmap[l][x][z];
                    if(occlude && h==w->heightmap[l][x+1][z] && h==w->heightmap[l][x+1][z+1] && h==w->heightmap[l][x][z+1])
                        w->occludemap[l][x][z]|=0x924;
                }
                int shape=0,rotation=0,texture=-1,ovl=-2,fg=0;
                if(over>0 && over<=flo_cfg->count) {
                    FloType *f=&flo_cfg->instances[over-1];
                    shape=w->tile_overlay_shapes[l][x][z]+1;
                    rotation=w->tile_overlay_rotations[l][x][z];
                    texture=f->texture;
                    if(texture>=0) {ovl=-1;fg=pix3d_texture_average(texture);}
                    else if(f->rgb!=0xff00ff) {
                        ovl=flo_hsl24to16(f->hue,f->saturation,f->lightness);
                        fg=pix3d_colour_table[overlay_color(f->hsl,96)];
                    }
                }
                world3d_set_tile(world3d,l,x,z,shape,rotation,texture,
                    w->heightmap[l][x][z],w->heightmap[l][x+1][z],
                    w->heightmap[l][x+1][z+1],w->heightmap[l][x][z+1],
                    underlay_color(base,light[x][z]),underlay_color(base,light[x+1][z]),
                    underlay_color(base,light[x+1][z+1]),underlay_color(base,light[x][z+1]),
                    overlay_color(ovl,light[x][z]),overlay_color(ovl,light[x+1][z]),
                    overlay_color(ovl,light[x+1][z+1]),overlay_color(ovl,light[x][z+1]),bg,fg);
            }
        }
    }
}

int world_location_layer(int shape) {return shape<4 ? 0:shape<9 ? 1:shape==22 ? 3:2;}
bool world_change_location(World *w,World3D *scene,LocConfig *cfg,CollisionMap *cm,int level,int x,int z,int id,int shape,int rotation) {
    if(!w || !scene || !cfg || level<0 || level>=4 || x<=0 || z<=0 || x>=w->max_tile_x-1 || z>=w->max_tile_z-1 || shape<0 || shape>22) return false;
    scene->structure_revision++;
    int layer=world_location_layer(shape),plane=level-((w->tile_flags[1][x][z]&2) ? 1:0);
    int old_id=-1;
    for(size_t i=0;i<scene->map_location_count;i++) {
        WorldMapLocation old=scene->map_locations[i];
        if(old.level!=level || old.x!=x || old.z!=z || world_location_layer(old.shape)!=layer) continue;
        old_id=old.id;LocType type;
        if(cm && plane>=0 && loc_get(cfg,old.id,&type)) {
            if(type.blockwalk) {
                if(old.shape<4) collision_map_del_wall(&cm[plane],x,z,old.shape,old.rotation,type.blockrange);
                else if(old.shape==22) {if(type.active) collision_map_block_ground(&cm[plane],x,z,false);}
                else if(old.shape>=9) collision_map_del_loc(&cm[plane],x,z,(old.rotation&1)?type.length:type.width,(old.rotation&1)?type.width:type.length,type.blockrange);
            }
            loc_free_entry(&type);
        }
        memmove(&scene->map_locations[i],&scene->map_locations[i+1],(scene->map_location_count-i-1)*sizeof(old));scene->map_location_count--;break;
    }
    GroundTile *tile=&scene->tiles[level][x][z];
    for(int i=0;i<tile->loc_count;) {
        WorldLocation *loc=&tile->locs[i];
        if(((loc->bitset>>14)&32767)!=old_id || world_location_layer(loc->shape)!=layer) {i++;continue;}
        if(loc->owns_model) model_free_unshared(loc->model);
        memmove(loc,loc+1,(tile->loc_count-i-1)*sizeof(*loc));tile->loc_count--;
    }
    return id<0 || place_location(w,cfg,scene,cm,level,x,z,id,shape,rotation,false);
}
