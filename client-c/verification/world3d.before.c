#include "dash3d/world3d.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include <string.h>
#include <stdio.h>

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

void world3d_reset(World3D *w) {
    w->taking_input = false;
    w->click_tile_x = -1;
    w->click_tile_z = -1;
    for (int l = 0; l < w->max_level; l++) {
        for (int x = 0; x < w->max_tile_x; x++) {
            for (int z = 0; z < w->max_tile_z; z++) {
                GroundTile *t = &w->tiles[l][x][z];
                t->level = l;
                t->x = x;
                t->z = z;
                t->has_underlay = false;
                t->has_overlay = false;
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

    int shape_idx = shape - 1;
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
    loc->min_tile_x = x;
    loc->max_tile_x = x + width - 1;
    loc->min_tile_z = z;
    loc->max_tile_z = z + length - 1;
    return true;
}

static void project_point(int vx, int vy, int vz, int eyeX, int eyeY, int eyeZ,
                          int sin_p, int cos_p, int sin_y, int cos_y,
                          int *out_sx, int *out_sy, int *out_sz) {
    int dx = vx - eyeX;
    int dy = vy - eyeY;
    int dz = vz - eyeZ;

    // Yaw rotation (horizontal)
    int x1 = (dx * cos_y + dz * sin_y) >> 16;
    int z1 = (dz * cos_y - dx * sin_y) >> 16;

    // Pitch rotation (vertical)
    int y2 = (dy * cos_p - z1 * sin_p) >> 16;
    int z2 = (dy * sin_p + z1 * cos_p) >> 16;

    if (z2 < 50) {
        *out_sz = -1;
        return;
    }

    *out_sx = pix3d_center_x + (x1 << 9) / z2;
    *out_sy = pix3d_center_y + (y2 << 9) / z2;
    *out_sz = z2;
}

void world3d_render(World3D *w, int eyeX, int eyeY, int eyeZ, int pitch, int yaw, int roll, int plane) {
    (void)roll;
    int sin_p = pix3d_sin_table[pitch & 2047];
    int cos_p = pix3d_cos_table[pitch & 2047];
    int sin_y = pix3d_sin_table[yaw & 2047];
    int cos_y = pix3d_cos_table[yaw & 2047];

    int center_tx = eyeX / 128;
    int center_tz = eyeZ / 128;
    int range = 25; // visible tile radius

    int min_tx = center_tx - range; if (min_tx < 0) min_tx = 0;
    int max_tx = center_tx + range; if (max_tx >= w->max_tile_x) max_tx = w->max_tile_x - 1;
    int min_tz = center_tz - range; if (min_tz < 0) min_tz = 0;
    int max_tz = center_tz + range; if (max_tz >= w->max_tile_z) max_tz = w->max_tile_z - 1;

    for (int l = 0; l <= plane && l < w->max_level; l++) {
        // 1. Render terrain tiles
        for (int tx = min_tx; tx <= max_tx; tx++) {
            for (int tz = min_tz; tz <= max_tz; tz++) {
                GroundTile *tile = &w->tiles[l][tx][tz];

                if (tile->has_underlay) {
                    TileUnderlay *u = &tile->underlay;
                    int x0 = tx * 128, z0 = tz * 128;
                    int x1 = x0 + 128, z1 = z0 + 128;
                    int y_sw = w->heightmap[l][tx][tz];
                    int y_se = w->heightmap[l][tx + 1][tz];
                    int y_ne = w->heightmap[l][tx + 1][tz + 1];
                    int y_nw = w->heightmap[l][tx][tz + 1];

                    int sx[4], sy[4], sz[4];
                    project_point(x0, y_sw, z0, eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y, &sx[0], &sy[0], &sz[0]);
                    project_point(x1, y_se, z0, eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y, &sx[1], &sy[1], &sz[1]);
                    project_point(x1, y_ne, z1, eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y, &sx[2], &sy[2], &sz[2]);
                    project_point(x0, y_nw, z1, eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y, &sx[3], &sy[3], &sz[3]);

                    if (sz[0] > 0 && sz[1] > 0 && sz[2] > 0 && sz[3] > 0) {
                        if (w->taking_input) {
                            if (world3d_point_inside_triangle(w->mouse_x, w->mouse_y, sy[2], sy[3], sy[1], sx[2], sx[3], sx[1]) ||
                                world3d_point_inside_triangle(w->mouse_x, w->mouse_y, sy[0], sy[1], sy[3], sx[0], sx[1], sx[3])) {
                                w->click_tile_x = tx;
                                w->click_tile_z = tz;
                            }
                        }

                        // Java World3D.java: Triangle 1: NE (2), NW (3), SE (1)
                        if ((sx[2] - sx[3]) * (sy[1] - sy[3]) - (sy[2] - sy[3]) * (sx[1] - sx[3]) > 0) {
                            if (u->northeast_color != 12345678) {
                                pix3d_gouraud_triangle(sx[2], sx[3], sx[1], sy[2], sy[3], sy[1],
                                                       u->northeast_color << 8, u->northwest_color << 8, u->southeast_color << 8);
                            }
                        }
                        // Java World3D.java: Triangle 2: SW (0), SE (1), NW (3)
                        if ((sx[0] - sx[1]) * (sy[3] - sy[1]) - (sy[0] - sy[1]) * (sx[3] - sx[1]) > 0) {
                            if (u->southwest_color != 12345678) {
                                pix3d_gouraud_triangle(sx[0], sx[1], sx[3], sy[0], sy[1], sy[3],
                                                       u->southwest_color << 8, u->southeast_color << 8, u->northwest_color << 8);
                            }
                        }
                    }
                }

                if (tile->has_overlay) {
                    TileOverlay *ovl = &tile->overlay;
                    int sx[10], sy[10], sz[10];
                    bool all_visible = true;

                    for (int v = 0; v < ovl->vertex_count; v++) {
                        project_point(ovl->vertex_x[v], ovl->vertex_y[v], ovl->vertex_z[v],
                                      eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y,
                                      &sx[v], &sy[v], &sz[v]);
                        if (sz[v] <= 0) all_visible = false;
                    }

                    if (all_visible) {
                        for (int t = 0; t < ovl->triangle_count; t++) {
                            int va = ovl->triangle_vertex_a[t];
                            int vb = ovl->triangle_vertex_b[t];
                            int vc = ovl->triangle_vertex_c[t];

                            // Java World3D.java: TileOverlay winding test: (x_a - x_b)*(y_c - y_b) - (y_a - y_b)*(x_c - x_b) > 0
                            if ((sx[va] - sx[vb]) * (sy[vc] - sy[vb]) - (sy[va] - sy[vb]) * (sx[vc] - sx[vb]) > 0) {
                                if (w->taking_input) {
                                    if (world3d_point_inside_triangle(w->mouse_x, w->mouse_y, sy[va], sy[vb], sy[vc], sx[va], sx[vb], sx[vc])) {
                                        w->click_tile_x = tx;
                                        w->click_tile_z = tz;
                                    }
                                }
                                if (ovl->triangle_color_a[t] != 12345678) {
                                    pix3d_gouraud_triangle(sx[va], sx[vb], sx[vc], sy[va], sy[vb], sy[vc],
                                                           ovl->triangle_color_a[t] << 8, ovl->triangle_color_b[t] << 8, ovl->triangle_color_c[t] << 8);
                                }
                            }
                        }
                    }
                }

                // 2. Render scenery objects on tile
                for (int i = 0; i < tile->loc_count; i++) {
                    WorldLocation *loc = &tile->locs[i];
                    if (loc->model) {
                        model_draw(loc->model, 0, pitch, yaw, loc->x - eyeX, loc->y - eyeY, loc->z - eyeZ);
                    }
                }
            }
        }
    }
}
