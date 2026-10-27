#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/anim.h"
#include "core/packet.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    int32_t num_points;
    int32_t num_faces;
    int32_t num_t;

    int32_t vertex_order_offset;
    int32_t vertex_x_offset;
    int32_t vertex_y_offset;
    int32_t vertex_z_offset;
    int32_t vertex_label_offset;

    int32_t face_index_offset;
    int32_t face_index_order_offset;
    int32_t face_colour_offset;
    int32_t face_render_type_offset;
    int32_t face_priority_offset;
    int32_t face_alpha_offset;
    int32_t face_label_offset;
    int32_t face_texture_axis_offset;
} ModelMeta;

static ModelMeta *g_model_meta = NULL;
static int32_t    g_model_meta_count = 0;

static Packet g_head;
static Packet g_face1;
static Packet g_face2;
static Packet g_face3;
static Packet g_face4;
static Packet g_face5;
static Packet g_point1;
static Packet g_point2;
static Packet g_point3;
static Packet g_point4;
static Packet g_point5;
static Packet g_vertex1;
static Packet g_vertex2;
static Packet g_axis;

// Transform buffers
static int32_t g_screen_x[4096];
static int32_t g_screen_y[4096];
static int32_t g_screen_z[4096];

static Packet load_packet(Jagfile *jag, const char *name, Arena *arena) {
    size_t size = 0;
    uint8_t *data = jagfile_read(jag, name, NULL, &size, arena);
    if (!data) {
        fprintf(stderr, "[Model] Failed to read %s\n", name);
        return packet_wrap(NULL, 0);
    }
    return packet_wrap(data, size);
}

void model_init_repo(Jagfile *models_jag, Arena *arena) {
    g_head    = load_packet(models_jag, "ob_head.dat", arena);
    g_face1   = load_packet(models_jag, "ob_face1.dat", arena);
    g_face2   = load_packet(models_jag, "ob_face2.dat", arena);
    g_face3   = load_packet(models_jag, "ob_face3.dat", arena);
    g_face4   = load_packet(models_jag, "ob_face4.dat", arena);
    g_face5   = load_packet(models_jag, "ob_face5.dat", arena);
    g_point1  = load_packet(models_jag, "ob_point1.dat", arena);
    g_point2  = load_packet(models_jag, "ob_point2.dat", arena);
    g_point3  = load_packet(models_jag, "ob_point3.dat", arena);
    g_point4  = load_packet(models_jag, "ob_point4.dat", arena);
    g_point5  = load_packet(models_jag, "ob_point5.dat", arena);
    g_vertex1 = load_packet(models_jag, "ob_vertex1.dat", arena);
    g_vertex2 = load_packet(models_jag, "ob_vertex2.dat", arena);
    g_axis    = load_packet(models_jag, "ob_axis.dat", arena);

    g_head.pos = 0;
    g_point1.pos = 0;
    g_point2.pos = 0;
    g_point3.pos = 0;
    g_point4.pos = 0;
    g_vertex1.pos = 0;
    g_vertex2.pos = 0;

    int32_t total = g2(&g_head);
    g_model_meta_count = total + 100;

    g_model_meta = arena ? (ModelMeta *)arena_alloc_zero(arena, g_model_meta_count * sizeof(ModelMeta))
                         : (ModelMeta *)calloc(g_model_meta_count, sizeof(ModelMeta));

    int32_t v_label_off = 0;
    int32_t f_color_off = 0;
    int32_t f_info_off = 0;
    int32_t f_prio_off = 0;
    int32_t f_alpha_off = 0;
    int32_t f_label_off = 0;
    int32_t f_tex_off = 0;

    for (int32_t i = 0; i < total; i++) {
        int32_t id = g2(&g_head);
        ModelMeta *meta = &g_model_meta[id];

        meta->num_points = g2(&g_head);
        meta->num_faces  = g2(&g_head);
        meta->num_t      = g1(&g_head);

        meta->vertex_order_offset = (int32_t)g_point1.pos;
        meta->vertex_x_offset     = (int32_t)g_point2.pos;
        meta->vertex_y_offset     = (int32_t)g_point3.pos;
        meta->vertex_z_offset     = (int32_t)g_point4.pos;
        meta->face_index_offset   = (int32_t)g_vertex1.pos;
        meta->face_index_order_offset = (int32_t)g_vertex2.pos;

        int32_t has_info  = g1(&g_head);
        int32_t prio_val  = g1(&g_head);
        int32_t has_alpha = g1(&g_head);
        int32_t has_flab  = g1(&g_head);
        int32_t has_vlab  = g1(&g_head);

        for (int32_t v = 0; v < meta->num_points; v++) {
            int32_t order = g1(&g_point1);
            if (order & 1) gsmart(&g_point2);
            if (order & 2) gsmart(&g_point3);
            if (order & 4) gsmart(&g_point4);
        }

        for (int32_t f = 0; f < meta->num_faces; f++) {
            int32_t type = g1(&g_vertex2);
            if (type == 1) {
                gsmart(&g_vertex1);
                gsmart(&g_vertex1);
            }
            gsmart(&g_vertex1);
        }

        meta->face_colour_offset = f_color_off;
        f_color_off += meta->num_faces * 2;

        meta->face_render_type_offset = (has_info == 1) ? f_info_off : -1;
        if (has_info == 1) f_info_off += meta->num_faces;

        meta->face_priority_offset = (prio_val == 255) ? f_prio_off : (-prio_val - 1);
        if (prio_val == 255) f_prio_off += meta->num_faces;

        meta->face_alpha_offset = (has_alpha == 1) ? f_alpha_off : -1;
        if (has_alpha == 1) f_alpha_off += meta->num_faces;

        meta->face_label_offset = (has_flab == 1) ? f_label_off : -1;
        if (has_flab == 1) f_label_off += meta->num_faces;

        meta->vertex_label_offset = (has_vlab == 1) ? v_label_off : -1;
        if (has_vlab == 1) v_label_off += meta->num_points;

        meta->face_texture_axis_offset = f_tex_off;
        f_tex_off += meta->num_t;
    }
}

Model *model_load(int32_t id, Arena *arena) {
    if (!g_model_meta || id < 0 || id >= g_model_meta_count) return NULL;
    ModelMeta *meta = &g_model_meta[id];
    if (meta->num_points == 0 && meta->num_faces == 0) return NULL;

    Model *m = arena ? (Model *)arena_alloc_zero(arena, sizeof(Model))
                     : (Model *)calloc(1, sizeof(Model));
    if (!m) return NULL;

    m->num_points = meta->num_points;
    m->num_faces  = meta->num_faces;
    m->num_t      = meta->num_t;

    m->point_x = arena ? (int32_t *)arena_alloc(arena, m->num_points * sizeof(int32_t))
                       : (int32_t *)malloc(m->num_points * sizeof(int32_t));
    m->point_y = arena ? (int32_t *)arena_alloc(arena, m->num_points * sizeof(int32_t))
                       : (int32_t *)malloc(m->num_points * sizeof(int32_t));
    m->point_z = arena ? (int32_t *)arena_alloc(arena, m->num_points * sizeof(int32_t))
                       : (int32_t *)malloc(m->num_points * sizeof(int32_t));

    m->face_vertex_a = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                             : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    m->face_vertex_b = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                             : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    m->face_vertex_c = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                             : (int32_t *)malloc(m->num_faces * sizeof(int32_t));

    m->face_colour = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                           : (int32_t *)malloc(m->num_faces * sizeof(int32_t));

    if (meta->face_render_type_offset >= 0) {
        m->face_render_type = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                                    : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    }
    if (meta->face_priority_offset >= 0) {
        m->face_priority = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                                 : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    } else {
        m->priority = -meta->face_priority_offset - 1;
    }
    if (meta->face_alpha_offset >= 0) {
        m->face_alpha = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                              : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    }
    if (meta->vertex_label_offset >= 0) {
        m->vertex_label = arena ? (int32_t *)arena_alloc(arena, m->num_points * sizeof(int32_t))
                                : (int32_t *)malloc(m->num_points * sizeof(int32_t));
    }
    if (meta->face_label_offset >= 0) {
        m->face_label = arena ? (int32_t *)arena_alloc(arena, m->num_faces * sizeof(int32_t))
                              : (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    }

    // Read vertices
    g_point1.pos = (size_t)meta->vertex_order_offset;
    g_point2.pos = (size_t)meta->vertex_x_offset;
    g_point3.pos = (size_t)meta->vertex_y_offset;
    g_point4.pos = (size_t)meta->vertex_z_offset;
    if (meta->vertex_label_offset >= 0) {
        g_point5.pos = (size_t)meta->vertex_label_offset;
    }

    int32_t dx = 0, dy = 0, dz = 0;
    for (int32_t v = 0; v < m->num_points; v++) {
        int32_t order = g1(&g_point1);
        int32_t x = (order & 1) ? gsmart(&g_point2) : 0;
        int32_t y = (order & 2) ? gsmart(&g_point3) : 0;
        int32_t z = (order & 4) ? gsmart(&g_point4) : 0;

        m->point_x[v] = dx + x;
        m->point_y[v] = dy + y;
        m->point_z[v] = dz + z;

        dx = m->point_x[v];
        dy = m->point_y[v];
        dz = m->point_z[v];

        if (m->vertex_label) {
            m->vertex_label[v] = g1(&g_point5);
        }
    }

    // Read face colors & types
    g_face1.pos = (size_t)meta->face_colour_offset;
    if (meta->face_render_type_offset >= 0) g_face2.pos = (size_t)meta->face_render_type_offset;
    if (meta->face_priority_offset >= 0)    g_face3.pos = (size_t)meta->face_priority_offset;
    if (meta->face_alpha_offset >= 0)       g_face4.pos = (size_t)meta->face_alpha_offset;
    if (meta->face_label_offset >= 0)       g_face5.pos = (size_t)meta->face_label_offset;

    for (int32_t f = 0; f < m->num_faces; f++) {
        m->face_colour[f] = g2(&g_face1);
        if (m->face_render_type) m->face_render_type[f] = g1(&g_face2);
        if (m->face_priority)    m->face_priority[f] = g1(&g_face3);
        if (m->face_alpha)       m->face_alpha[f] = g1(&g_face4);
        if (m->face_label)       m->face_label[f] = g1(&g_face5);
    }

    // Read face vertices
    g_vertex1.pos = (size_t)meta->face_index_offset;
    g_vertex2.pos = (size_t)meta->face_index_order_offset;

    int32_t a = 0, b = 0, c = 0, last = 0;
    for (int32_t f = 0; f < m->num_faces; f++) {
        int32_t type = g1(&g_vertex2);
        if (type == 1) {
            a = gsmart(&g_vertex1) + last;
            b = gsmart(&g_vertex1) + a;
            c = gsmart(&g_vertex1) + b;
            last = c;
        } else if (type == 2) {
            b = c;
            c = gsmart(&g_vertex1) + last;
            last = c;
        } else if (type == 3) {
            a = c;
            c = gsmart(&g_vertex1) + last;
            last = c;
        } else if (type == 4) {
            int32_t t = a; a = b; b = t;
            c = gsmart(&g_vertex1) + last;
            last = c;
        }

        m->face_vertex_a[f] = a;
        m->face_vertex_b[f] = b;
        m->face_vertex_c[f] = c;
    }

    return m;
}

static int32_t get_shaded_colour(int32_t hsl, int32_t scalar) {
    scalar = (scalar * (hsl & 0x7F)) >> 7;
    if (scalar < 2) scalar = 2;
    else if (scalar > 126) scalar = 126;
    return (hsl & 0xFF80) + scalar;
}

void model_calculate_normals(Model *m, int32_t ambient, int32_t contrast, int32_t lx, int32_t ly, int32_t lz) {
    if (!m) return;

    m->face_colour_a = (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    m->face_colour_b = (int32_t *)malloc(m->num_faces * sizeof(int32_t));
    m->face_colour_c = (int32_t *)malloc(m->num_faces * sizeof(int32_t));

    int32_t light_mag = (int32_t)sqrt(lx * lx + ly * ly + lz * lz);
    int32_t scale = (contrast * light_mag) >> 8;
    if (scale <= 0) scale = 1;

    typedef struct { int32_t x, y, z, w; } Norm;
    Norm *norms = (Norm *)calloc(m->num_points, sizeof(Norm));

    for (int32_t f = 0; f < m->num_faces; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        int32_t dxAB = m->point_x[b] - m->point_x[a];
        int32_t dyAB = m->point_y[b] - m->point_y[a];
        int32_t dzAB = m->point_z[b] - m->point_z[a];

        int32_t dxAC = m->point_x[c] - m->point_x[a];
        int32_t dyAC = m->point_y[c] - m->point_y[a];
        int32_t dzAC = m->point_z[c] - m->point_z[a];

        int32_t nx = dyAB * dzAC - dyAC * dzAB;
        int32_t ny = dzAB * dxAC - dzAC * dxAB;
        int32_t nz = dxAB * dyAC - dxAC * dyAB;

        while (nx > 8192 || ny > 8192 || nz > 8192 || nx < -8192 || ny < -8192 || nz < -8192) {
            nx >>= 1; ny >>= 1; nz >>= 1;
        }

        int32_t len = (int32_t)sqrt(nx * nx + ny * ny + nz * nz);
        if (len <= 0) len = 1;

        nx = (nx * 256) / len;
        ny = (ny * 256) / len;
        nz = (nz * 256) / len;

        norms[a].x += nx; norms[a].y += ny; norms[a].z += nz; norms[a].w++;
        norms[b].x += nx; norms[b].y += ny; norms[b].z += nz; norms[b].w++;
        norms[c].x += nx; norms[c].y += ny; norms[c].z += nz; norms[c].w++;
    }

    for (int32_t f = 0; f < m->num_faces; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];
        int32_t col = m->face_colour[f];

        Norm *na = &norms[a];
        Norm *nb = &norms[b];
        Norm *nc = &norms[c];

        int32_t lightA = ambient + ((na->w > 0) ? ((lx * na->x + ly * na->y + lz * na->z) / (scale * na->w)) : 0);
        int32_t lightB = ambient + ((nb->w > 0) ? ((lx * nb->x + ly * nb->y + lz * nb->z) / (scale * nb->w)) : 0);
        int32_t lightC = ambient + ((nc->w > 0) ? ((lx * nc->x + ly * nc->y + lz * nc->z) / (scale * nc->w)) : 0);

        m->face_colour_a[f] = get_shaded_colour(col, lightA);
        m->face_colour_b[f] = get_shaded_colour(col, lightB);
        m->face_colour_c[f] = get_shaded_colour(col, lightC);
    }

    free(norms);
}

void model_draw(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz) {
    if (!m) return;

    int32_t sinM = pix3d_sin_table[yaw & 2047];
    int32_t cosM = pix3d_cos_table[yaw & 2047];
    int32_t sinP = pix3d_sin_table[eye_pitch & 2047];
    int32_t cosP = pix3d_cos_table[eye_pitch & 2047];
    int32_t sinY = pix3d_sin_table[eye_yaw & 2047];
    int32_t cosY = pix3d_cos_table[eye_yaw & 2047];

    for (int32_t v = 0; v < m->num_points && v < 4096; v++) {
        int32_t x = m->point_x[v];
        int32_t y = m->point_y[v];
        int32_t z = m->point_z[v];

        if (yaw != 0) {
            int32_t tx = (z * sinM + x * cosM) >> 16;
            z = (z * cosM - x * sinM) >> 16;
            x = tx;
        }

        x += dx;
        y += dy;
        z += dz;

        int32_t tx = (z * sinY + x * cosY) >> 16;
        z = (z * cosY - x * sinY) >> 16;
        x = tx;

        int32_t ty = (y * cosP - z * sinP) >> 16;
        z = (y * sinP + z * cosP) >> 16;
        y = ty;

        if (z < 50) {
            g_screen_z[v] = -1;
            g_screen_x[v] = -5000;
            g_screen_y[v] = -5000;
        } else {
            g_screen_z[v] = z;
            g_screen_x[v] = pix3d_center_x + ((x << 9) / z);
            g_screen_y[v] = pix3d_center_y + ((y << 9) / z);
        }
    }

    int32_t face_order[4096];
    int32_t face_depth[4096];
    int32_t visible_faces = 0;

    for (int32_t f = 0; f < m->num_faces && visible_faces < 4096; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        if (g_screen_z[a] <= 0 || g_screen_z[b] <= 0 || g_screen_z[c] <= 0) continue;

        int32_t xA = g_screen_x[a];
        int32_t xB = g_screen_x[b];
        int32_t xC = g_screen_x[c];

        int32_t yA = g_screen_y[a];
        int32_t yB = g_screen_y[b];
        int32_t yC = g_screen_y[c];

        int32_t dxAB = xA - xB;
        int32_t dyAB = yA - yB;
        int32_t dxCB = xC - xB;
        int32_t dyCB = yC - yB;

        if (dxAB * dyCB - dyAB * dxCB <= 0) {
            continue;
        }

        face_order[visible_faces] = f;
        face_depth[visible_faces] = (g_screen_z[a] + g_screen_z[b] + g_screen_z[c]) / 3;
        visible_faces++;
    }

    // Sort back-to-front (Painter's algorithm)
    for (int32_t i = 1; i < visible_faces; i++) {
        int32_t cur_f = face_order[i];
        int32_t cur_d = face_depth[i];
        int32_t j = i - 1;
        while (j >= 0 && face_depth[j] < cur_d) {
            face_order[j + 1] = face_order[j];
            face_depth[j + 1] = face_depth[j];
            j--;
        }
        face_order[j + 1] = cur_f;
        face_depth[j + 1] = cur_d;
    }

    // Rasterize sorted triangles with full 16-bit HSL << 8
    for (int32_t i = 0; i < visible_faces; i++) {
        int32_t f = face_order[i];
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        pix3d_gouraud_triangle(
            g_screen_x[a], g_screen_x[b], g_screen_x[c],
            g_screen_y[a], g_screen_y[b], g_screen_y[c],
            m->face_colour_a[f] << 8, m->face_colour_b[f] << 8, m->face_colour_c[f] << 8
        );
    }
}

bool model_draw_pickable(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz, int mouse_x, int mouse_y) {
    if (!m) return false;
    model_draw(m, yaw, eye_pitch, eye_yaw, dx, dy, dz);

    if (mouse_x < 0 || mouse_y < 0) return false;

    for (int32_t f = 0; f < m->num_faces; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        if (g_screen_z[a] <= 0 || g_screen_z[b] <= 0 || g_screen_z[c] <= 0) continue;

        int32_t xA = g_screen_x[a], xB = g_screen_x[b], xC = g_screen_x[c];
        int32_t yA = g_screen_y[a], yB = g_screen_y[b], yC = g_screen_y[c];

        int32_t dxAB = xA - xB, dyAB = yA - yB;
        int32_t dxCB = xC - xB, dyCB = yC - yB;
        if (dxAB * dyCB - dyAB * dxCB <= 0) continue;

        int32_t c1 = (xB - xA) * (mouse_y - yA) - (yB - yA) * (mouse_x - xA);
        int32_t c2 = (xC - xB) * (mouse_y - yB) - (yC - yB) * (mouse_x - xB);
        int32_t c3 = (xA - xC) * (mouse_y - yC) - (yA - yC) * (mouse_x - xC);

        if ((c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0)) {
            return true;
        }
    }
    return false;
}

void model_render_obj(Model *m, int32_t pitch, int32_t yaw, int32_t roll, int32_t eye_pitch, int32_t eye_x, int32_t eye_y, int32_t eye_z) {
    if (!m) return;

    int32_t sinPitch = pix3d_sin_table[pitch & 2047];
    int32_t cosPitch = pix3d_cos_table[pitch & 2047];
    int32_t sinYaw   = pix3d_sin_table[yaw & 2047];
    int32_t cosYaw   = pix3d_cos_table[yaw & 2047];
    int32_t sinRoll  = pix3d_sin_table[roll & 2047];
    int32_t cosRoll  = pix3d_cos_table[roll & 2047];
    int32_t sinEye   = pix3d_sin_table[eye_pitch & 2047];
    int32_t cosEye   = pix3d_cos_table[eye_pitch & 2047];

    for (int32_t v = 0; v < m->num_points && v < 4096; v++) {
        int32_t x = m->point_x[v];
        int32_t y = m->point_y[v];
        int32_t z = m->point_z[v];

        if (roll != 0) {
            int32_t tx = (y * sinRoll + x * cosRoll) >> 16;
            y = (y * cosRoll - x * sinRoll) >> 16;
            x = tx;
        }
        if (pitch != 0) {
            int32_t ty = (y * cosPitch - z * sinPitch) >> 16;
            z = (y * sinPitch + z * cosPitch) >> 16;
            y = ty;
        }
        if (yaw != 0) {
            int32_t tx = (z * sinYaw + x * cosYaw) >> 16;
            z = (z * cosYaw - x * sinYaw) >> 16;
            x = tx;
        }

        x += eye_x;
        y += eye_y;
        z += eye_z;

        int32_t ty = (y * cosEye - z * sinEye) >> 16;
        z = (y * sinEye + z * cosEye) >> 16;
        y = ty;

        if (z <= 50) z = 50;

        g_screen_z[v] = z;
        g_screen_x[v] = pix3d_center_x + ((x << 9) / z);
        g_screen_y[v] = pix3d_center_y + ((y << 9) / z);
    }

    // Collect visible faces with backface culling
    int32_t face_order[4096];
    int32_t face_depth[4096];
    int32_t visible_faces = 0;

    for (int32_t f = 0; f < m->num_faces && visible_faces < 4096; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        int32_t xA = g_screen_x[a];
        int32_t xB = g_screen_x[b];
        int32_t xC = g_screen_x[c];

        int32_t yA = g_screen_y[a];
        int32_t yB = g_screen_y[b];
        int32_t yC = g_screen_y[c];

        int32_t dxAB = xA - xB;
        int32_t dyAB = yA - yB;
        int32_t dxCB = xC - xB;
        int32_t dyCB = yC - yB;

        // Backface culling
        if (dxAB * dyCB - dyAB * dxCB <= 0) {
            continue;
        }

        face_order[visible_faces] = f;
        face_depth[visible_faces] = (g_screen_z[a] + g_screen_z[b] + g_screen_z[c]) / 3;
        visible_faces++;
    }

    // Sort back-to-front (Painter's algorithm)
    for (int32_t i = 1; i < visible_faces; i++) {
        int32_t cur_f = face_order[i];
        int32_t cur_d = face_depth[i];
        int32_t j = i - 1;
        while (j >= 0 && face_depth[j] < cur_d) {
            face_order[j + 1] = face_order[j];
            face_depth[j + 1] = face_depth[j];
            j--;
        }
        face_order[j + 1] = cur_f;
        face_depth[j + 1] = cur_d;
    }

    // Rasterize sorted triangles
    for (int32_t i = 0; i < visible_faces; i++) {
        int32_t f = face_order[i];
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        pix3d_gouraud_triangle(
            g_screen_x[a], g_screen_x[b], g_screen_x[c],
            g_screen_y[a], g_screen_y[b], g_screen_y[c],
            m->face_colour_a[f] << 8, m->face_colour_b[f] << 8, m->face_colour_c[f] << 8
        );
    }
}

void model_recolor(Model *m, int32_t src_color, int32_t dst_color) {
    if (!m || !m->face_colour) return;
    for (int32_t i = 0; i < m->num_faces; i++) {
        if (m->face_colour[i] == src_color) {
            m->face_colour[i] = dst_color;
        }
    }
}

void model_translate(Model *m, int32_t y, int32_t x, int32_t z) {
    if (!m) return;
    for (int32_t i = 0; i < m->num_points; i++) {
        m->point_x[i] += x;
        m->point_y[i] += y;
        m->point_z[i] += z;
    }
}

Model *model_copy(const Model *src, Arena *arena) {
    if (!src) return NULL;
    Model *dst = arena ? (Model*)arena_alloc(arena, sizeof(Model))
                       : (Model*)malloc(sizeof(Model));
    memset(dst, 0, sizeof(Model));

    dst->num_points = src->num_points;
    dst->num_faces = src->num_faces;
    dst->priority = src->priority;
    dst->min_depth = src->min_depth;
    dst->max_depth = src->max_depth;

    if (src->num_points > 0) {
        dst->point_x = arena ? (int32_t*)arena_alloc(arena, src->num_points * sizeof(int32_t))
                             : (int32_t*)malloc(src->num_points * sizeof(int32_t));
        dst->point_y = arena ? (int32_t*)arena_alloc(arena, src->num_points * sizeof(int32_t))
                             : (int32_t*)malloc(src->num_points * sizeof(int32_t));
        dst->point_z = arena ? (int32_t*)arena_alloc(arena, src->num_points * sizeof(int32_t))
                             : (int32_t*)malloc(src->num_points * sizeof(int32_t));
        memcpy(dst->point_x, src->point_x, src->num_points * sizeof(int32_t));
        memcpy(dst->point_y, src->point_y, src->num_points * sizeof(int32_t));
        memcpy(dst->point_z, src->point_z, src->num_points * sizeof(int32_t));

        if (src->vertex_label) {
            dst->vertex_label = arena ? (int32_t*)arena_alloc(arena, src->num_points * sizeof(int32_t))
                                      : (int32_t*)malloc(src->num_points * sizeof(int32_t));
            memcpy(dst->vertex_label, src->vertex_label, src->num_points * sizeof(int32_t));
        }
    }

    if (src->num_faces > 0) {
        dst->face_vertex_a = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_vertex_b = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_vertex_c = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_colour   = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_colour_a = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_colour_b = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
        dst->face_colour_c = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                   : (int32_t*)malloc(src->num_faces * sizeof(int32_t));

        memcpy(dst->face_vertex_a, src->face_vertex_a, src->num_faces * sizeof(int32_t));
        memcpy(dst->face_vertex_b, src->face_vertex_b, src->num_faces * sizeof(int32_t));
        memcpy(dst->face_vertex_c, src->face_vertex_c, src->num_faces * sizeof(int32_t));
        memcpy(dst->face_colour, src->face_colour, src->num_faces * sizeof(int32_t));
        if (src->face_colour_a) memcpy(dst->face_colour_a, src->face_colour_a, src->num_faces * sizeof(int32_t));
        if (src->face_colour_b) memcpy(dst->face_colour_b, src->face_colour_b, src->num_faces * sizeof(int32_t));
        if (src->face_colour_c) memcpy(dst->face_colour_c, src->face_colour_c, src->num_faces * sizeof(int32_t));

        if (src->face_render_type) {
            dst->face_render_type = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                          : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
            memcpy(dst->face_render_type, src->face_render_type, src->num_faces * sizeof(int32_t));
        }
        if (src->face_priority) {
            dst->face_priority = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                       : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
            memcpy(dst->face_priority, src->face_priority, src->num_faces * sizeof(int32_t));
        }
        if (src->face_alpha) {
            dst->face_alpha = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                    : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
            memcpy(dst->face_alpha, src->face_alpha, src->num_faces * sizeof(int32_t));
        }
        if (src->face_label) {
            dst->face_label = arena ? (int32_t*)arena_alloc(arena, src->num_faces * sizeof(int32_t))
                                    : (int32_t*)malloc(src->num_faces * sizeof(int32_t));
            memcpy(dst->face_label, src->face_label, src->num_faces * sizeof(int32_t));
        }
    }

    if (src->label_vertices) {
        dst->label_vertices = src->label_vertices;
        dst->label_vertex_counts = src->label_vertex_counts;
        dst->num_label_vertices = src->num_label_vertices;
    } else if (dst->vertex_label) {
        model_create_label_references(dst, arena);
    }
    return dst;
}

static int add_composite_vertex(Model *dst, Model *src, int src_idx) {
    int x = src->point_x[src_idx];
    int y = src->point_y[src_idx];
    int z = src->point_z[src_idx];
    for (int i = 0; i < dst->num_points; i++) {
        if (dst->point_x[i] == x && dst->point_y[i] == y && dst->point_z[i] == z) {
            return i;
        }
    }
    int idx = dst->num_points++;
    dst->point_x[idx] = x;
    dst->point_y[idx] = y;
    dst->point_z[idx] = z;
    if (src->vertex_label && dst->vertex_label) {
        dst->vertex_label[idx] = src->vertex_label[src_idx];
    }
    return idx;
}

Model *model_create_composite(Model **models, int32_t count, Arena *arena) {
    if (!models || count <= 0) return NULL;
    int32_t total_points = 0;
    int32_t total_faces = 0;

    for (int32_t i = 0; i < count; i++) {
        if (models[i]) {
            total_points += models[i]->num_points;
            total_faces += models[i]->num_faces;
        }
    }
    if (total_faces == 0) return NULL;

    Model *m = (Model *)arena_alloc(arena, sizeof(Model));
    memset(m, 0, sizeof(Model));

    m->point_x = (int32_t *)arena_alloc(arena, total_points * sizeof(int32_t));
    m->point_y = (int32_t *)arena_alloc(arena, total_points * sizeof(int32_t));
    m->point_z = (int32_t *)arena_alloc(arena, total_points * sizeof(int32_t));
    m->vertex_label = (int32_t *)arena_alloc_zero(arena, total_points * sizeof(int32_t));

    m->face_vertex_a = (int32_t *)arena_alloc(arena, total_faces * sizeof(int32_t));
    m->face_vertex_b = (int32_t *)arena_alloc(arena, total_faces * sizeof(int32_t));
    m->face_vertex_c = (int32_t *)arena_alloc(arena, total_faces * sizeof(int32_t));
    m->face_colour = (int32_t *)arena_alloc(arena, total_faces * sizeof(int32_t));
    m->face_render_type = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));
    m->face_priority = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));
    m->face_alpha = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));
    m->face_label = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));

    m->num_points = 0;
    m->num_faces = 0;

    for (int32_t i = 0; i < count; i++) {
        Model *src = models[i];
        if (!src) continue;
        for (int32_t f = 0; f < src->num_faces; f++) {
            int out_f = m->num_faces++;
            m->face_colour[out_f] = src->face_colour[f];
            if (src->face_render_type) m->face_render_type[out_f] = src->face_render_type[f];
            if (src->face_priority) m->face_priority[out_f] = src->face_priority[f];
            if (src->face_alpha) m->face_alpha[out_f] = src->face_alpha[f];
            if (src->face_label) m->face_label[out_f] = src->face_label[f];

            m->face_vertex_a[out_f] = add_composite_vertex(m, src, src->face_vertex_a[f]);
            m->face_vertex_b[out_f] = add_composite_vertex(m, src, src->face_vertex_b[f]);
            m->face_vertex_c[out_f] = add_composite_vertex(m, src, src->face_vertex_c[f]);
        }
    }
    return m;
}

static int g_base_x = 0;
static int g_base_y = 0;
static int g_base_z = 0;

void model_create_label_references(Model *m, Arena *arena) {
    if (!m || !m->vertex_label) return;

    int counts[256] = {0};
    int max_lbl = 0;
    for (int i = 0; i < m->num_points; i++) {
        int lbl = m->vertex_label[i];
        if (lbl >= 0 && lbl < 256) {
            counts[lbl]++;
            if (lbl > max_lbl) max_lbl = lbl;
        }
    }

    m->num_label_vertices = max_lbl + 1;
    m->label_vertex_counts = (int32_t *)arena_alloc(arena, m->num_label_vertices * sizeof(int32_t));
    m->label_vertices = (int32_t **)arena_alloc(arena, m->num_label_vertices * sizeof(int32_t *));

    for (int i = 0; i <= max_lbl; i++) {
        m->label_vertex_counts[i] = counts[i];
        if (counts[i] > 0) {
            m->label_vertices[i] = (int32_t *)arena_alloc(arena, counts[i] * sizeof(int32_t));
        } else {
            m->label_vertices[i] = NULL;
        }
        counts[i] = 0;
    }

    for (int i = 0; i < m->num_points; i++) {
        int lbl = m->vertex_label[i];
        if (lbl >= 0 && lbl <= max_lbl) {
            m->label_vertices[lbl][counts[lbl]++] = i;
        }
    }
}

static void apply_single_transform(Model *m, int type, const int *labels, int label_count, int dx, int dy, int dz) {
    if (!m || !m->label_vertices || !labels) return;

    if (type == 0) {
        int count = 0;
        g_base_x = 0;
        g_base_y = 0;
        g_base_z = 0;
        for (int i = 0; i < label_count; i++) {
            int lbl = labels[i];
            if (lbl < m->num_label_vertices && m->label_vertices[lbl]) {
                int v_cnt = m->label_vertex_counts[lbl];
                const int32_t *v_list = m->label_vertices[lbl];
                for (int j = 0; j < v_cnt; j++) {
                    int v = v_list[j];
                    g_base_x += m->point_x[v];
                    g_base_y += m->point_y[v];
                    g_base_z += m->point_z[v];
                    count++;
                }
            }
        }
        if (count > 0) {
            g_base_x = g_base_x / count + dx;
            g_base_y = g_base_y / count + dy;
            g_base_z = g_base_z / count + dz;
        } else {
            g_base_x = dx;
            g_base_y = dy;
            g_base_z = dz;
        }
        return;
    }

    if (type == 1) { // Translation
        for (int i = 0; i < label_count; i++) {
            int lbl = labels[i];
            if (lbl < m->num_label_vertices && m->label_vertices[lbl]) {
                int v_cnt = m->label_vertex_counts[lbl];
                const int32_t *v_list = m->label_vertices[lbl];
                for (int j = 0; j < v_cnt; j++) {
                    int v = v_list[j];
                    m->point_x[v] += dx;
                    m->point_y[v] += dy;
                    m->point_z[v] += dz;
                }
            }
        }
    } else if (type == 2) { // Rotation
        for (int i = 0; i < label_count; i++) {
            int lbl = labels[i];
            if (lbl < m->num_label_vertices && m->label_vertices[lbl]) {
                int v_cnt = m->label_vertex_counts[lbl];
                const int32_t *v_list = m->label_vertices[lbl];
                for (int j = 0; j < v_cnt; j++) {
                    int v = v_list[j];
                    m->point_x[v] -= g_base_x;
                    m->point_y[v] -= g_base_y;
                    m->point_z[v] -= g_base_z;

                    int xan = (dx & 0xFF) * 8;
                    int yan = (dy & 0xFF) * 8;
                    int zan = (dz & 0xFF) * 8;

                    if (zan != 0) {
                        int sin_val = pix3d_sin_table[zan];
                        int cos_val = pix3d_cos_table[zan];
                        int tmp = (m->point_y[v] * sin_val + m->point_x[v] * cos_val) >> 16;
                        m->point_y[v] = (m->point_y[v] * cos_val - m->point_x[v] * sin_val) >> 16;
                        m->point_x[v] = tmp;
                    }
                    if (xan != 0) {
                        int sin_val = pix3d_sin_table[xan];
                        int cos_val = pix3d_cos_table[xan];
                        int tmp = (m->point_y[v] * cos_val - m->point_z[v] * sin_val) >> 16;
                        m->point_z[v] = (m->point_y[v] * sin_val + m->point_z[v] * cos_val) >> 16;
                        m->point_y[v] = tmp;
                    }
                    if (yan != 0) {
                        int sin_val = pix3d_sin_table[yan];
                        int cos_val = pix3d_cos_table[yan];
                        int tmp = (m->point_z[v] * sin_val + m->point_x[v] * cos_val) >> 16;
                        m->point_z[v] = (m->point_z[v] * cos_val - m->point_x[v] * sin_val) >> 16;
                        m->point_x[v] = tmp;
                    }

                    m->point_x[v] += g_base_x;
                    m->point_y[v] += g_base_y;
                    m->point_z[v] += g_base_z;
                }
            }
        }
    } else if (type == 3) { // Scaling
        for (int i = 0; i < label_count; i++) {
            int lbl = labels[i];
            if (lbl < m->num_label_vertices && m->label_vertices[lbl]) {
                int v_cnt = m->label_vertex_counts[lbl];
                const int32_t *v_list = m->label_vertices[lbl];
                for (int j = 0; j < v_cnt; j++) {
                    int v = v_list[j];
                    m->point_x[v] -= g_base_x;
                    m->point_y[v] -= g_base_y;
                    m->point_z[v] -= g_base_z;

                    m->point_x[v] = m->point_x[v] * dx / 128;
                    m->point_y[v] = m->point_y[v] * dy / 128;
                    m->point_z[v] = m->point_z[v] * dz / 128;

                    m->point_x[v] += g_base_x;
                    m->point_y[v] += g_base_y;
                    m->point_z[v] += g_base_z;
                }
            }
        }
    }
}

void model_apply_transform(Model *m, int frame_id) {
    if (!m || !m->label_vertices || frame_id == -1) return;
    AnimFrame *f = anim_get_frame(frame_id);
    if (!f || !f->base) return;

    g_base_x = 0;
    g_base_y = 0;
    g_base_z = 0;

    for (int i = 0; i < f->length; i++) {
        int grp = f->groups[i];
        apply_single_transform(m, f->base->types[grp], f->base->labels[grp], f->base->label_counts[grp], f->x[i], f->y[i], f->z[i]);
    }
}

void model_apply_transforms(Model *m, int primary_frame, int secondary_frame, const int *mask) {
    if (primary_frame == -1) return;
    if (!mask || secondary_frame == -1) {
        model_apply_transform(m, primary_frame);
        return;
    }

    AnimFrame *f1 = anim_get_frame(primary_frame);
    AnimFrame *f2 = anim_get_frame(secondary_frame);
    if (!f1 || !f1->base || !f2 || !f2->base) return;

    g_base_x = 0;
    g_base_y = 0;
    g_base_z = 0;

    int mask_idx = 0;
    int mask_val = mask[mask_idx++];

    for (int i = 0; i < f1->length; i++) {
        int grp = f1->groups[i];
        while (grp > mask_val && mask_val != -1) {
            mask_val = mask[mask_idx++];
        }
        if (grp != mask_val || f1->base->types[grp] == 0) {
            apply_single_transform(m, f1->base->types[grp], f1->base->labels[grp], f1->base->label_counts[grp], f1->x[i], f1->y[i], f1->z[i]);
        }
    }

    g_base_x = 0;
    g_base_y = 0;
    g_base_z = 0;

    mask_idx = 0;
    mask_val = mask[mask_idx++];

    for (int i = 0; i < f2->length; i++) {
        int grp = f2->groups[i];
        while (grp > mask_val && mask_val != -1) {
            mask_val = mask[mask_idx++];
        }
        if (grp == mask_val || f2->base->types[grp] == 0) {
            apply_single_transform(m, f2->base->types[grp], f2->base->labels[grp], f2->base->label_counts[grp], f2->x[i], f2->y[i], f2->z[i]);
        }
    }
}
