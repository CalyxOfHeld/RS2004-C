#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { FB_W = 256, FB_H = 256, BACKDROP = 0x667788 };

static uint32_t framebuffer[FB_W * FB_H];
static uint32_t detail_reference[FB_W * FB_H];

static bool face_is_flat_at_y(const Model *m, int f, int y) {
    int a = m->face_vertex_a[f], b = m->face_vertex_b[f], c = m->face_vertex_c[f];
    return a >= 0 && b >= 0 && c >= 0 && a < m->num_points && b < m->num_points &&
           c < m->num_points && m->point_y[a] == y && m->point_y[b] == y &&
           m->point_y[c] == y;
}

// Find a real cache model with priority-1 black decoration over priority-0
// colored ground. Model IDs are discovered from the local cache instead of
// embedding a scenery/model ID in the regression.
static Model *find_coplanar_cache_detail(Arena *arena, int *found_id) {
    for (int id = 0; id < 4096; ++id) {
        Model *m = model_load(id, arena);
        if (!m || !m->face_priority || !m->face_colour) continue;
        for (int detail = 0; detail < m->num_faces; ++detail) {
            int y = m->point_y[m->face_vertex_a[detail]];
            if (m->face_priority[detail] != 1 || m->face_colour[detail] != 0 ||
                !face_is_flat_at_y(m, detail, y)) continue;
            for (int base = 0; base < m->num_faces; ++base) {
                if (m->face_priority[base] == 0 && m->face_colour[base] != 0 &&
                    face_is_flat_at_y(m, base, y)) {
                    *found_id = id;
                    return m;
                }
            }
        }
    }
    return NULL;
}

static int count_black_pixels(void) {
    int black = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) black += framebuffer[i] == 0;
    return black;
}

static Model priority_black_subset(const Model *source,
                                   int32_t *a, int32_t *b, int32_t *c,
                                   int32_t *color, int32_t *priority,
                                   int32_t *type) {
    Model subset = *source;
    int count = 0;
    for (int f = 0; f < source->num_faces; ++f) {
        if (!source->face_priority || source->face_priority[f] != 1 ||
            !source->face_colour || source->face_colour[f] != 0 ||
            (source->face_render_type && source->face_render_type[f] == 255)) continue;
        a[count] = source->face_vertex_a[f];
        b[count] = source->face_vertex_b[f];
        c[count] = source->face_vertex_c[f];
        color[count] = source->face_colour[f];
        priority[count] = source->face_priority[f];
        if (source->face_render_type) type[count] = source->face_render_type[f];
        ++count;
    }
    subset.num_faces = count;
    subset.face_vertex_a = a;
    subset.face_vertex_b = b;
    subset.face_vertex_c = c;
    subset.face_colour = color;
    subset.face_colour_a = NULL;
    subset.face_colour_b = NULL;
    subset.face_colour_c = NULL;
    subset.face_priority = priority;
    subset.face_render_type = source->face_render_type ? type : NULL;
    return subset;
}

static void clear_scene(void) {
    for (int i = 0; i < FB_W * FB_H; ++i) framebuffer[i] = BACKDROP;
    pix2d_bind(framebuffer, FB_W, FB_H);
    pix3d_init_2d();
    pix3d_center_x = FB_W / 2;
    pix3d_center_y = FB_H / 2;
    pix3d_begin_scene();
}

static void camera_offset(int yaw, int side_shift, int distance,
                          int *dx, int *dy, int *dz) {
    int32_t sin_yaw = pix3d_sin_table[yaw & 2047];
    int32_t cos_yaw = pix3d_cos_table[yaw & 2047];
    // Inverse camera yaw: put the model in front of the camera, with a small
    // lateral translation that moves the projection across adjacent samples.
    *dx = (int)(((int64_t)cos_yaw * side_shift - (int64_t)sin_yaw * distance) >> 16);
    *dz = (int)(((int64_t)sin_yaw * side_shift + (int64_t)cos_yaw * distance) >> 16);
    *dy = distance;
}

static void verify_cache_detail(Model *m, int *worst_black, int *views) {
    const int yaws[] = { 0, 512, 1024, 1536 };
    int32_t a[4096], b[4096], c[4096], colors[4096], priorities[4096], types[4096];
    Model detail = priority_black_subset(m, a, b, c, colors, priorities, types);
    assert(detail.num_faces > 0);
    for (int q = 0; q < 4; ++q) {
        for (int shift = -3; shift <= 3; ++shift) {
            int dx, dy, dz;
            camera_offset(yaws[q], shift, 600, &dx, &dy, &dz);
            clear_scene();
            model_draw(&detail, 0, 256, yaws[q], dx, dy, dz);
            pix3d_end_scene();
            for (int i = 0; i < FB_W * FB_H; ++i)
                detail_reference[i] = framebuffer[i];

            clear_scene();
            model_draw(m, 0, 256, yaws[q], dx, dy, dz);
            pix3d_end_scene();
            int black = count_black_pixels();
            if (black < *worst_black) *worst_black = black;
            assert(black >= 12 && "cache model priority-1 black detail disappeared");
            for (int i = 0; i < FB_W * FB_H; ++i)
                assert(detail_reference[i] != 0 || framebuffer[i] == 0);
            ++*views;
        }
    }
}

static void verify_separate_model_occlusion(void) {
    // A small nearer model is drawn between the cache asset and a farther
    // model. Depth must preserve the nearer model even when the far model is
    // submitted last. These are separate model draws, matching scene objects.
    int32_t px[] = { -20, 20, 20, -20 };
    int32_t py[] = { 0, 0, 0, 0 };
    int32_t pz[] = { -20, -20, 20, 20 };
    int32_t fa[] = { 2, 0 }, fb[] = { 3, 1 }, fc[] = { 1, 3 };
    int32_t near_colors[] = { 3, 3 }, far_colors[] = { 4, 4 };
    Model near_model = {0}, far_model = {0};
    near_model.num_points = far_model.num_points = 4;
    near_model.num_faces = far_model.num_faces = 2;
    near_model.point_x = far_model.point_x = px;
    near_model.point_y = far_model.point_y = py;
    near_model.point_z = far_model.point_z = pz;
    near_model.face_vertex_a = far_model.face_vertex_a = fa;
    near_model.face_vertex_b = far_model.face_vertex_b = fb;
    near_model.face_vertex_c = far_model.face_vertex_c = fc;
    near_model.face_colour = near_model.face_colour_a = near_colors;
    far_model.face_colour = far_model.face_colour_a = far_colors;
    pix3d_colour_table[3] = 0x00ff00;
    pix3d_colour_table[4] = 0x0000ff;

    int dx, dy, dz;
    camera_offset(0, 0, 580, &dx, &dy, &dz);
    clear_scene();
    model_draw(&near_model, 0, 256, 0, dx, dy, dz);
    camera_offset(0, 0, 620, &dx, &dy, &dz);
    model_draw(&far_model, 0, 256, 0, dx, dy, dz);
    pix3d_end_scene();

    int green = 0, blue = 0;
    for (int y = 118; y < 138; ++y) {
        for (int x = 118; x < 138; ++x) {
            uint32_t c = framebuffer[y * FB_W + x];
            green += c == 0x00ff00;
            blue += c == 0x0000ff;
        }
    }
    assert(green > 20 && "nearby model was not rasterized in the overlap");
    assert(blue == 0 && "farther model overwrote the nearer model");
}

static void verify_face_priority_over_coplanar_base(void) {
    // Deliberately submit the black priority-1 detail before the white
    // priority-0 base. Both are exactly coplanar; the cache priority contract
    // requires the detail to remain visible after the base is drawn.
    int32_t px[] = { -64, 64, 64, -64, -20, 20, 20, -20 };
    int32_t py[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int32_t pz[] = { -64, -64, 64, 64, -20, -20, 20, 20 };
    int32_t fa[] = { 6, 4, 2, 0 };
    int32_t fb[] = { 7, 5, 3, 1 };
    int32_t fc[] = { 5, 7, 1, 3 };
    int32_t colors[] = { 0, 0, 1, 1 };
    int32_t priorities[] = { 1, 1, 0, 0 };
    Model model = {0};
    model.num_points = 8;
    model.num_faces = 4;
    model.point_x = px;
    model.point_y = py;
    model.point_z = pz;
    model.face_vertex_a = fa;
    model.face_vertex_b = fb;
    model.face_vertex_c = fc;
    model.face_colour = model.face_colour_a = colors;
    model.face_priority = priorities;
    pix3d_colour_table[0] = 0x000000;
    pix3d_colour_table[1] = 0xffffff;

    Model detail_only = model;
    detail_only.num_faces = 2;
    detail_only.face_vertex_a = fa;
    detail_only.face_vertex_b = fb;
    detail_only.face_vertex_c = fc;
    detail_only.face_colour = detail_only.face_colour_a = colors;
    detail_only.face_priority = priorities;

    for (int yaw = 0; yaw < 2048; yaw += 512) {
        for (int shift = -3; shift <= 3; ++shift) {
            int dx, dy, dz;
            camera_offset(yaw, shift, 600, &dx, &dy, &dz);
            clear_scene();
            model_draw(&detail_only, 0, 256, yaw, dx, dy, dz);
            pix3d_end_scene();
            for (int i = 0; i < FB_W * FB_H; ++i)
                detail_reference[i] = framebuffer[i];

            clear_scene();
            model_draw(&model, 0, 256, yaw, dx, dy, dz);
            pix3d_end_scene();
            int expected_detail = 0;
            for (int i = 0; i < FB_W * FB_H; ++i) {
                if (detail_reference[i] != 0) continue;
                ++expected_detail;
                assert(framebuffer[i] == 0 &&
                       "coplanar priority-0 base overwrote priority-1 decoration");
            }
            assert(expected_detail >= 12 && "synthetic detail fixture projected too small");
        }
    }
}

static void verify_composite_priority_copy(Arena *arena) {
    int32_t x[] = { -8, 8, 0 }, y[] = { 0, 0, 0 }, z[] = { 0, 0, 8 };
    int32_t a[] = { 0 }, b[] = { 1 }, c[] = { 2 }, color[] = { 100 };
    int32_t detail_priority[] = { 1 };
    Model constant = {0}, per_face = {0};
    Model *parts[] = { &constant, &per_face };
    Model *combined;

    constant.num_points = per_face.num_points = 3;
    constant.num_faces = per_face.num_faces = 1;
    constant.point_x = per_face.point_x = x;
    constant.point_y = per_face.point_y = y;
    constant.point_z = per_face.point_z = z;
    constant.face_vertex_a = per_face.face_vertex_a = a;
    constant.face_vertex_b = per_face.face_vertex_b = b;
    constant.face_vertex_c = per_face.face_vertex_c = c;
    constant.face_colour = per_face.face_colour = color;
    constant.priority = 6;
    per_face.face_priority = detail_priority;

    combined = model_create_composite(parts, 2, arena);
    assert(combined && combined->face_priority);
    assert(combined->face_priority[0] == 6);
    assert(combined->face_priority[1] == 1);
}

int main(void) {
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_colour_table[0] = 0x000000;
    pix3d_colour_table[3] = 0x00ff00;
    pix3d_colour_table[4] = 0x0000ff;

    Arena arena = arena_create(32 * 1024 * 1024);
    Jagfile *models = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models) models = jagfile_load_file("engine/data/pack/client/models", &arena);
    assert(models && "local revision-225 model cache could not be loaded");
    model_init_repo(models, &arena);

    int model_id = -1;
    Model *fixture = find_coplanar_cache_detail(&arena, &model_id);
    assert(fixture && "cache has no coplanar priority-1 black detail candidate");
    int worst_black = FB_W * FB_H, views = 0;
    verify_cache_detail(fixture, &worst_black, &views);
    verify_face_priority_over_coplanar_base();
    verify_composite_priority_copy(&arena);
    verify_separate_model_occlusion();

    printf("PASS: cache model %d retained coplanar priority-1 detail in %d yaw/shift views (minimum %d black pixels)\n",
           model_id, views, worst_black);
    puts("PASS: a farther separately drawn model did not overwrite nearer geometry");
    arena_free(&arena);
    return 0;
}
