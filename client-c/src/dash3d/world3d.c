#include "dash3d/world3d.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include "ui/mouse_picking.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const int SHAPE_POINTS[13][8] = {
    { 1, 3, 5, 7, 0, 0, 0, 0 },
    { 1, 3, 5, 7, 0, 0, 0, 0 },
    { 1, 3, 5, 7, 0, 0, 0, 0 },
    { 1, 3, 5, 7, 6, 0, 0, 0 },
    { 1, 3, 5, 7, 6, 0, 0, 0 },
    { 1, 3, 5, 7, 6, 0, 0, 0 },
    { 1, 3, 5, 7, 6, 0, 0, 0 },
    { 1, 3, 5, 7, 2, 6, 0, 0 },
    { 1, 3, 5, 7, 2, 8, 0, 0 },
    { 1, 3, 5, 7, 2, 8, 0, 0 },
    { 1, 3, 5, 7, 11, 12, 0, 0 },
    { 1, 3, 5, 7, 11, 12, 0, 0 },
    { 1, 3, 5, 7, 13, 14, 0, 0 }
};

static const int SHAPE_POINT_COUNTS[13] = { 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6 };

static const int SHAPE_PATHS[13][24] = {
    { 0, 1, 2, 3, 0, 0, 1, 3 },
    { 1, 1, 2, 3, 1, 0, 1, 3 },
    { 0, 1, 2, 3, 1, 0, 1, 3 },
    { 0, 0, 1, 2, 0, 0, 2, 4, 1, 0, 4, 3 },
    { 0, 0, 1, 4, 0, 0, 4, 3, 1, 1, 2, 4 },
    { 0, 0, 4, 3, 1, 0, 1, 2, 1, 0, 2, 4 },
    { 0, 1, 2, 4, 1, 0, 1, 4, 1, 0, 4, 3 },
    { 0, 4, 1, 2, 0, 4, 2, 5, 1, 0, 4, 5, 1, 0, 5, 3 },
    { 0, 4, 1, 2, 0, 4, 2, 3, 0, 4, 3, 5, 1, 0, 4, 5 },
    { 0, 0, 4, 5, 1, 4, 1, 2, 1, 4, 2, 3, 1, 4, 3, 5 },
    { 0, 0, 1, 5, 0, 1, 4, 5, 0, 1, 2, 4, 1, 0, 5, 3, 1, 5, 4, 3, 1, 4, 2, 3 },
    { 1, 0, 1, 5, 1, 1, 4, 5, 1, 1, 2, 4, 0, 0, 5, 3, 0, 5, 4, 3, 0, 4, 2, 3 },
    { 1, 0, 5, 4, 1, 0, 1, 5, 0, 0, 4, 3, 0, 4, 5, 3, 0, 5, 2, 3, 0, 1, 2, 5 }
};

static const int SHAPE_PATH_COUNTS[13] = { 8, 8, 8, 12, 12, 12, 12, 16, 16, 16, 24, 24, 24 };

void world3d_init(World3D *w, int max_x, int max_z, int max_level) {
    memset(w, 0, sizeof(World3D));
    w->max_tile_x = max_x > WORLD3D_MAX_X ? WORLD3D_MAX_X : max_x;
    w->max_tile_z = max_z > WORLD3D_MAX_Z ? WORLD3D_MAX_Z : max_z;
    w->max_level = max_level > WORLD3D_MAX_LEVEL ? WORLD3D_MAX_LEVEL : max_level;
    w->taking_input = false;
    w->click_tile_x = -1;
    w->click_tile_z = -1;
    world3d_reset(w);
}

void world3d_dispose(World3D *w) {
    world3d_reset(w);
    free(w->map_locations);w->map_locations=NULL;
    w->map_location_count=w->map_location_capacity=0;
    free(w->location_animations);w->location_animations=NULL;
    w->location_animation_capacity=0;
}
bool world3d_add_map_location(World3D *w,WorldMapLocation loc) {
    if(w->map_location_count==w->map_location_capacity) {
        size_t cap=w->map_location_capacity ? w->map_location_capacity*2:1024;
        WorldMapLocation *p=realloc(w->map_locations,cap*sizeof(*p));
        if(!p) {fprintf(stderr,"[Scene] Cannot retain map location metadata\n");return false;}
        w->map_locations=p;w->map_location_capacity=cap;
    }
    w->map_locations[w->map_location_count++]=loc;return true;
}
void world3d_reset(World3D *w) {
    w->map_location_count=0;
    w->location_animation_count=0;
    w->taking_input = false;
    w->click_tile_x = -1;
    w->click_tile_z = -1;
    for (int l = 0; l < w->max_level; l++) {
        for (int x = 0; x < w->max_tile_x; x++) {
            for (int z = 0; z < w->max_tile_z; z++) {
                GroundTile *t = &w->tiles[l][x][z];
                ground_items_clear(t);
                t->level = l;
                t->x = x;
                t->z = z;
                t->has_underlay = false;
                t->has_overlay = false;
                for(int i=0;i<t->loc_count;i++) if(t->locs[i].owns_model) model_free_unshared(t->locs[i].model);
                t->loc_count = 0;
            }
        }
    }
}

void world3d_click(World3D *w, int mouse_x, int mouse_y) {
    if (!w) return;
    w->taking_input = true;
    w->mouse_x = mouse_x;
    w->mouse_y = mouse_y;
    w->click_tile_x = -1;
    w->click_tile_z = -1;
}

static inline bool world3d_point_inside_triangle(int mouse_x, int mouse_y,
                                                 int yA, int yB, int yC,
                                                 int xA, int xB, int xC) {
    if (mouse_y < yA && mouse_y < yB && mouse_y < yC) return false;
    if (mouse_y > yA && mouse_y > yB && mouse_y > yC) return false;
    if (mouse_x < xA && mouse_x < xB && mouse_x < xC) return false;
    if (mouse_x > xA && mouse_x > xB && mouse_x > xC) return false;

    int64_t var9  = (int64_t)(mouse_y - yA) * (xB - xA) - (int64_t)(mouse_x - xA) * (yB - yA);
    int64_t var10 = (int64_t)(mouse_y - yC) * (xA - xC) - (int64_t)(mouse_x - xC) * (yA - yC);
    int64_t var11 = (int64_t)(mouse_y - yB) * (xC - xB) - (int64_t)(mouse_x - xB) * (yC - yB);
    return (var9 * var11 > 0 && var11 * var10 > 0);
}

void world3d_set_tile(World3D *w, int level, int x, int z, int shape, int rotation, int texture_id,
                      int y_sw, int y_se, int y_ne, int y_nw,
                      int hsl_sw, int hsl_se, int hsl_ne, int hsl_nw,
                      int ovl_hsl_sw, int ovl_hsl_se, int ovl_hsl_ne, int ovl_hsl_nw,
                      int rgb_bg, int rgb_fg) {
    if (level < 0 || level >= w->max_level || x < 0 || x >= w->max_tile_x || z < 0 || z >= w->max_tile_z) {
        return;
    }

    GroundTile *tile = &w->tiles[level][x][z];

    if (shape == 0) {
        tile->has_underlay = true;
        tile->underlay.southwest_color = hsl_sw;
        tile->underlay.southeast_color = hsl_se;
        tile->underlay.northeast_color = hsl_ne;
        tile->underlay.northwest_color = hsl_nw;
        tile->underlay.texture_id = texture_id;
        tile->underlay.rgb = rgb_bg;
        tile->underlay.flat = (y_sw == y_se && y_sw == y_ne && y_sw == y_nw);
        return;
    }

    if (shape == 1) {
        tile->has_underlay = true;
        tile->underlay.southwest_color = ovl_hsl_sw;
        tile->underlay.southeast_color = ovl_hsl_se;
        tile->underlay.northeast_color = ovl_hsl_ne;
        tile->underlay.northwest_color = ovl_hsl_nw;
        tile->underlay.texture_id = texture_id;
        tile->underlay.rgb = rgb_fg;
        tile->underlay.flat = (y_sw == y_se && y_sw == y_ne && y_sw == y_nw);
        return;
    }

    // Overlay (shape >= 2)
    tile->has_overlay = true;
    TileOverlay *ovl = &tile->overlay;
    ovl->shape = shape;
    ovl->rotation = rotation;
    ovl->background_rgb = rgb_bg;
    ovl->foreground_rgb = rgb_fg;
    ovl->triangle_texture_id = texture_id;
    ovl->flat = (y_sw == y_se && y_sw == y_ne && y_sw == y_nw);

    // Ground.ts / TileOverlay.java index both definition tables by the scene
    // shape itself. Shapes 0/1 are handled by the underlay paths above.
    int shape_idx = shape;
    if (shape_idx < 0 || shape_idx >= 13) return;

    int pt_count = SHAPE_POINT_COUNTS[shape_idx];
    ovl->vertex_count = pt_count;

    int tile_size = 128;
    int half_size = 64;
    int q1 = 32;
    int q3 = 96;

    int base_x = x * tile_size;
    int base_z = z * tile_size;

    int under_cols[10];
    int over_cols[10];

    for (int i = 0; i < pt_count; i++) {
        int pt = SHAPE_POINTS[shape_idx][i];
        if ((pt & 1) == 0 && pt <= 8) {
            pt = ((pt - rotation - rotation - 1) & 7) + 1;
        } else if (pt > 8 && pt <= 12) {
            pt = ((pt - 9 - rotation) & 3) + 9;
        } else if (pt > 12 && pt <= 16) {
            pt = ((pt - 13 - rotation) & 3) + 13;
        }

        int vx = 0, vz = 0, vy = 0, col_u = 0, col_o = 0;
        if (pt == 1) {
            vx = base_x; vz = base_z; vy = y_sw; col_u = hsl_sw; col_o = ovl_hsl_sw;
        } else if (pt == 2) {
            vx = base_x + half_size; vz = base_z; vy = (y_sw + y_se) >> 1; col_u = (hsl_sw + hsl_se) >> 1; col_o = (ovl_hsl_sw + ovl_hsl_se) >> 1;
        } else if (pt == 3) {
            vx = base_x + tile_size; vz = base_z; vy = y_se; col_u = hsl_se; col_o = ovl_hsl_se;
        } else if (pt == 4) {
            vx = base_x + tile_size; vz = base_z + half_size; vy = (y_se + y_ne) >> 1; col_u = (hsl_se + hsl_ne) >> 1; col_o = (ovl_hsl_se + ovl_hsl_ne) >> 1;
        } else if (pt == 5) {
            vx = base_x + tile_size; vz = base_z + tile_size; vy = y_ne; col_u = hsl_ne; col_o = ovl_hsl_ne;
        } else if (pt == 6) {
            vx = base_x + half_size; vz = base_z + tile_size; vy = (y_ne + y_nw) >> 1; col_u = (hsl_ne + hsl_nw) >> 1; col_o = (ovl_hsl_ne + ovl_hsl_nw) >> 1;
        } else if (pt == 7) {
            vx = base_x; vz = base_z + tile_size; vy = y_nw; col_u = hsl_nw; col_o = ovl_hsl_nw;
        } else if (pt == 8) {
            vx = base_x; vz = base_z + half_size; vy = (y_nw + y_sw) >> 1; col_u = (hsl_nw + hsl_sw) >> 1; col_o = (ovl_hsl_nw + ovl_hsl_sw) >> 1;
        } else if (pt == 9) {
            vx = base_x + half_size; vz = base_z + q1; vy = (y_sw + y_se) >> 1; col_u = (hsl_sw + hsl_se) >> 1; col_o = (ovl_hsl_sw + ovl_hsl_se) >> 1;
        } else if (pt == 10) {
            vx = base_x + q3; vz = base_z + half_size; vy = (y_se + y_ne) >> 1; col_u = (hsl_se + hsl_ne) >> 1; col_o = (ovl_hsl_se + ovl_hsl_ne) >> 1;
        } else if (pt == 11) {
            vx = base_x + half_size; vz = base_z + q3; vy = (y_ne + y_nw) >> 1; col_u = (hsl_ne + hsl_nw) >> 1; col_o = (ovl_hsl_ne + ovl_hsl_nw) >> 1;
        } else if (pt == 12) {
            vx = base_x + q1; vz = base_z + half_size; vy = (y_nw + y_sw) >> 1; col_u = (hsl_nw + hsl_sw) >> 1; col_o = (ovl_hsl_nw + ovl_hsl_sw) >> 1;
        } else if (pt == 13) {
            vx = base_x + q1; vz = base_z + q1; vy = y_sw; col_u = hsl_sw; col_o = ovl_hsl_sw;
        } else if (pt == 14) {
            vx = base_x + q3; vz = base_z + q1; vy = y_se; col_u = hsl_se; col_o = ovl_hsl_se;
        } else if (pt == 15) {
            vx = base_x + q3; vz = base_z + q3; vy = y_ne; col_u = hsl_ne; col_o = ovl_hsl_ne;
        } else if (pt == 16) {
            vx = base_x + q1; vz = base_z + q3; vy = y_nw; col_u = hsl_nw; col_o = ovl_hsl_nw;
        }

        ovl->vertex_x[i] = vx;
        ovl->vertex_y[i] = vy;
        ovl->vertex_z[i] = vz;
        under_cols[i] = col_u;
        over_cols[i] = col_o;
    }

    int path_count = SHAPE_PATH_COUNTS[shape_idx];
    int tri_count = path_count / 4;
    ovl->triangle_count = tri_count;

    for (int t = 0; t < tri_count; t++) {
        int color_type = SHAPE_PATHS[shape_idx][t * 4];
        int va = SHAPE_PATHS[shape_idx][t * 4 + 1];
        int vb = SHAPE_PATHS[shape_idx][t * 4 + 2];
        int vc = SHAPE_PATHS[shape_idx][t * 4 + 3];

        if (va < 4) va = (va - rotation) & 3;
        if (vb < 4) vb = (vb - rotation) & 3;
        if (vc < 4) vc = (vc - rotation) & 3;

        ovl->triangle_vertex_a[t] = va;
        ovl->triangle_vertex_b[t] = vb;
        ovl->triangle_vertex_c[t] = vc;

        ovl->triangle_texture[t] = color_type == 0 ? -1 : texture_id;
        if (color_type == 0) {
            ovl->triangle_color_a[t] = under_cols[va];
            ovl->triangle_color_b[t] = under_cols[vb];
            ovl->triangle_color_c[t] = under_cols[vc];
        } else {
            ovl->triangle_color_a[t] = over_cols[va];
            ovl->triangle_color_b[t] = over_cols[vb];
            ovl->triangle_color_c[t] = over_cols[vc];
        }
    }
}

bool world3d_add_loc(World3D *w, int level, int x, int z, int y, Model *model, int bitset,
                     int width, int length) {
    if (!w || !model || level < 0 || level >= w->max_level || x < 0 || x >= w->max_tile_x || z < 0 || z >= w->max_tile_z) {
        return false;
    }

    GroundTile *tile = &w->tiles[level][x][z];
    if (tile->loc_count >= 5) return false;

    WorldLocation *loc = &tile->locs[tile->loc_count++];
    loc->level = level;
    loc->x = x * 128 + (width * 128) / 2;
    loc->z = z * 128 + (length * 128) / 2;
    loc->y = y;
    loc->model = model;
    loc->bitset = bitset;
    loc->shape=10;loc->owns_model=false;loc->yaw=0;
    loc->min_tile_x = x;
    loc->max_tile_x = x + width - 1;
    loc->min_tile_z = z;
    loc->max_tile_z = z + length - 1;
    return true;
}


static void ground_triangle(World3D *w, int tx,int tz,Pix3DVertex a,Pix3DVertex b,Pix3DVertex c,int texture) {
    // Keep picking restricted to visible front-facing ground triangles.
    if(w->taking_input && a.z>=50 && b.z>=50 && c.z>=50) {
        int ax=pix3d_center_x+a.x*512/a.z, ay=pix3d_center_y+a.y*512/a.z;
        int bx=pix3d_center_x+b.x*512/b.z, by=pix3d_center_y+b.y*512/b.z;
        int cx=pix3d_center_x+c.x*512/c.z, cy=pix3d_center_y+c.y*512/c.z;
        if((int64_t)(ax-bx)*(cy-by)-(int64_t)(ay-by)*(cx-bx)>0 &&
           world3d_point_inside_triangle(w->mouse_x,w->mouse_y,ay,by,cy,ax,bx,cx)) {
            w->click_tile_x=tx; w->click_tile_z=tz;
        }
    }
    if(texture>=0 || a.shade!=12345678) pix3d_scene_triangle(a,b,c,texture,0);
}
void world3d_render(World3D *w,int eyeX,int eyeY,int eyeZ,int pitch,int yaw,int roll,int plane) {
    (void)roll;
    pix3d_begin_scene();
    int minx=eyeX/128-25, maxx=eyeX/128+25, minz=eyeZ/128-25, maxz=eyeZ/128+25;
    if(minx<0) minx=0;
    if(minz<0) minz=0;
    if(maxx>=w->max_tile_x) maxx=w->max_tile_x-1;
    if(maxz>=w->max_tile_z) maxz=w->max_tile_z-1;
    for(int l=0;l<=plane && l<w->max_level;l++) {
        for(int x=minx;x<=maxx;x++) for(int z=minz;z<=maxz;z++) {
            GroundTile *tile=&w->tiles[l][x][z];
            if(tile->has_underlay) {
                TileUnderlay *u=&tile->underlay;
                Pix3DVertex v[4];
                int vx[4]={x,x+1,x+1,x}, vz[4]={z,z,z+1,z+1};
                int colors[4]={u->southwest_color,u->southeast_color,u->northeast_color,u->northwest_color};
                for(int i=0;i<4;i++) {
                    v[i]=pix3d_camera_vertex(vx[i]*128-eyeX,w->heightmap[l][vx[i]][vz[i]]-eyeY,vz[i]*128-eyeZ,pitch,yaw);
                    v[i].u=vx[i]-x; v[i].v=vz[i]-z; v[i].shade=colors[i];
                }
                ground_triangle(w,x,z,v[2],v[3],v[1],u->texture_id);
                ground_triangle(w,x,z,v[0],v[1],v[3],u->texture_id);
            }
            if(tile->has_overlay) {
                TileOverlay *o=&tile->overlay; Pix3DVertex v[10];
                for(int i=0;i<o->vertex_count;i++) {
                    v[i]=pix3d_camera_vertex(o->vertex_x[i]-eyeX,o->vertex_y[i]-eyeY,o->vertex_z[i]-eyeZ,pitch,yaw);
                    v[i].u=(o->vertex_x[i]-x*128)/128.0; v[i].v=(o->vertex_z[i]-z*128)/128.0;
                }
                for(int t=0;t<o->triangle_count;t++) {
                    Pix3DVertex a=v[o->triangle_vertex_a[t]], b=v[o->triangle_vertex_b[t]], c=v[o->triangle_vertex_c[t]];
                    a.shade=o->triangle_color_a[t]; b.shade=o->triangle_color_b[t]; c.shade=o->triangle_color_c[t];
                    ground_triangle(w,x,z,a,b,c,o->triangle_texture[t]);
                }
            }
        }
        for(int x=minx;x<=maxx;x++) for(int z=minz;z<=maxz;z++) {
            GroundTile *tile=&w->tiles[l][x][z];GroundItem *models[3];
            int count=ground_items_models(tile,models);
            int y=(w->heightmap[l][x][z]+w->heightmap[l][x+1][z]+w->heightmap[l][x][z+1]+w->heightmap[l][x+1][z+1])/4;
            int raise=0;
            for(int i=0;i<tile->loc_count;i++)
                if(tile->locs[i].shape>=9 && tile->locs[i].shape<=21 && tile->locs[i].model->obj_raise>raise) raise=tile->locs[i].model->obj_raise;
            y-=raise;
            bool picked=false;
            // Original draws the two lower distinct items before the top item.
            for(int i=1;i<=count;i++) {
                Model *model=models[i%count]->model;
                if(w->picker && w->picker->check_hover)
                    picked=model_draw_pickable(model,0,pitch,yaw,x*128+64-eyeX,y-eyeY,z*128+64-eyeZ,w->picker->mouse_x,w->picker->mouse_y)||picked;
                else model_draw(model,0,pitch,yaw,x*128+64-eyeX,y-eyeY,z*128+64-eyeZ);
            }
            // Picking any visible stack model exposes every item on that tile.
            if(picked) for(GroundItem *item=tile->items;item;item=item->next)
                mouse_picker_add_picked(w->picker,mouse_picker_make_bitset(3,item->id,x,z));
        }
        // Include a multi-tile object when its footprint intersects the range,
        // even when its anchor tile lies outside it. Each location is drawn once.
        for(int x=0;x<w->max_tile_x;x++) for(int z=0;z<w->max_tile_z;z++) {
            GroundTile *tile=&w->tiles[l][x][z];
            for(int i=0;i<tile->loc_count;i++) {
                WorldLocation *loc=&tile->locs[i];
                if(loc->max_tile_x<minx || loc->min_tile_x>maxx || loc->max_tile_z<minz || loc->min_tile_z>maxz) continue;
                if(w->picker && w->picker->check_hover) {
                    if(model_draw_pickable(loc->model,loc->yaw,pitch,yaw,loc->x-eyeX,loc->y-eyeY,loc->z-eyeZ,w->picker->mouse_x,w->picker->mouse_y))
                    {
                        uint32_t bitset=(2u<<29)|((uint32_t)((loc->bitset>>14)&32767)<<14)|((uint32_t)loc->min_tile_z<<7)|(uint32_t)loc->min_tile_x;
                        bool duplicate=false;
                        for(int picked=0;picked<w->picker->picked_count;picked++) if(w->picker->picked_bitsets[picked]==bitset) duplicate=true;
                        if(!duplicate && w->picker->picked_count<MAX_PICKED) w->picker->picked_bitsets[w->picker->picked_count++]=bitset;
                    }
                } else model_draw(loc->model,loc->yaw,pitch,yaw,loc->x-eyeX,loc->y-eyeY,loc->z-eyeZ);
            }
        }
    }
}
