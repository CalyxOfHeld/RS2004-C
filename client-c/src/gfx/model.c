#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_flat225.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix3d_texture225.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/anim.h"
#include "core/packet.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    bool present; // listed in ob_head.dat (Model.metadata[id] != null), even when empty
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

static Packet g_axis;
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

// Transform buffers
static Pix3DVertex g_camera[4096];
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
    g_axis    = load_packet(models_jag, "ob_axis.dat", arena);
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

        meta->present = true;
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
    if (!meta->present) return NULL;

    Model *m = arena ? (Model *)arena_alloc_zero(arena, sizeof(Model))
                     : (Model *)calloc(1, sizeof(Model));
    if (!m) return NULL;

    m->allocation_arena=arena;
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

    if (m->num_t) {
        m->texture_a = arena ? arena_alloc(arena, m->num_t * sizeof(int32_t)) : malloc(m->num_t * sizeof(int32_t));
        m->texture_b = arena ? arena_alloc(arena, m->num_t * sizeof(int32_t)) : malloc(m->num_t * sizeof(int32_t));
        m->texture_c = arena ? arena_alloc(arena, m->num_t * sizeof(int32_t)) : malloc(m->num_t * sizeof(int32_t));
        g_axis.pos = (size_t)meta->face_texture_axis_offset * 6;
        for (int t=0;t<m->num_t;t++) {
            m->texture_a[t]=g2(&g_axis); m->texture_b[t]=g2(&g_axis); m->texture_c[t]=g2(&g_axis);
        }
    }
    return m;
}

static int32_t get_shaded_colour(int32_t hsl, int32_t scalar, int type) {
    if (type & 2) {
        if (scalar < 0) scalar=0;
        if (scalar > 127) scalar=127;
        return 127-scalar;
 }
    scalar = (scalar * (hsl & 0x7F)) >> 7;
    if (scalar < 2) scalar = 2;
    else if (scalar > 126) scalar = 126;
    return (hsl & 0xFF80) + scalar;
}

static bool calculate_normals(Model *m,int32_t ambient,int32_t contrast,int32_t lx,int32_t ly,int32_t lz,bool deferred) {
    if (!m) return false;
    if(m->face_render_type_original) memcpy(m->face_render_type,m->face_render_type_original,m->num_faces*sizeof(int32_t));
    if(!m->allocation_arena) {free(m->vertex_normals);free(m->vertex_normals_original);free(m->face_render_type_original);}
    m->vertex_normals=m->vertex_normals_original=NULL;m->face_render_type_original=NULL;

    if(!m->face_colour_a) m->face_colour_a = m->allocation_arena ? arena_alloc_zero(m->allocation_arena,m->num_faces*sizeof(int32_t)) : calloc((size_t)m->num_faces,sizeof(int32_t));
    if(!m->face_colour_b) m->face_colour_b = m->allocation_arena ? arena_alloc_zero(m->allocation_arena,m->num_faces*sizeof(int32_t)) : calloc((size_t)m->num_faces,sizeof(int32_t));
    if(!m->face_colour_c) m->face_colour_c = m->allocation_arena ? arena_alloc_zero(m->allocation_arena,m->num_faces*sizeof(int32_t)) : calloc((size_t)m->num_faces,sizeof(int32_t));
    if(!m->face_colour_a || !m->face_colour_b || !m->face_colour_c) return false;
    if(deferred) {
        memset(m->face_colour_a,0,m->num_faces*sizeof(int32_t));
        memset(m->face_colour_b,0,m->num_faces*sizeof(int32_t));
        memset(m->face_colour_c,0,m->num_faces*sizeof(int32_t));
    }

    int32_t light_mag = (int32_t)sqrt(lx * lx + ly * ly + lz * lz);
    int32_t scale = (contrast * light_mag) >> 8;
    if (scale <= 0) scale = 1;

    ModelNormal *norms=deferred && m->allocation_arena ? arena_alloc(m->allocation_arena,m->num_points*sizeof(*norms)) : calloc(m->num_points,sizeof(*norms));
    if(!norms) return false;
    if(deferred && m->allocation_arena) memset(norms,0,m->num_points*sizeof(*norms));

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

        if (m->face_render_type && (m->face_render_type[f] & 1)) {
            int light=ambient+(lx*nx+ly*ny+lz*nz)/(scale+scale/2);
            m->face_colour_a[f]=get_shaded_colour(m->face_colour[f],light,m->face_render_type[f]);
            continue;
        }
        norms[a].x += nx; norms[a].y += ny; norms[a].z += nz; norms[a].w++;
        norms[b].x += nx; norms[b].y += ny; norms[b].z += nz; norms[b].w++;
        norms[c].x += nx; norms[c].y += ny; norms[c].z += nz; norms[c].w++;
    }

    if(deferred) {
        size_t ns=m->num_points*sizeof(*norms),fs=m->num_faces*sizeof(int32_t);
        ModelNormal *original=m->allocation_arena ? arena_alloc(m->allocation_arena,ns) : malloc(ns);
        int32_t *types=m->allocation_arena ? arena_alloc(m->allocation_arena,fs) : malloc(fs);
        if(!m->face_render_type) {
            m->face_render_type=m->allocation_arena ? arena_alloc(m->allocation_arena,fs) : malloc(fs);
            if(m->face_render_type) memset(m->face_render_type,0,fs);
        }
        if(!original || !types || !m->face_render_type) {
            if(!m->allocation_arena){free(norms);free(original);free(types);}
            return false;
        }
        m->vertex_normals=norms;m->vertex_normals_original=original;m->face_render_type_original=types;
        memcpy(original,norms,ns);memcpy(types,m->face_render_type,fs);
        for(int f=0;f<m->num_faces;f++) if(!(m->face_render_type[f]&1))
            m->face_colour_a[f]=m->face_colour_b[f]=m->face_colour_c[f]=0;
        return true;
    }
    for (int32_t f = 0; f < m->num_faces; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];
        int32_t col = m->face_colour[f];
        if (m->face_render_type && (m->face_render_type[f] & 1)) {
            /* Java leaves the two unused flat-face shade slots at zero. */
            m->face_colour_b[f]=m->face_colour_c[f]=0;
            continue;
        }

        ModelNormal *na = &norms[a];
        ModelNormal *nb = &norms[b];
        ModelNormal *nc = &norms[c];

        int32_t lightA = ambient + ((na->w > 0) ? ((lx * na->x + ly * na->y + lz * na->z) / (scale * na->w)) : 0);
        int32_t lightB = ambient + ((nb->w > 0) ? ((lx * nb->x + ly * nb->y + lz * nb->z) / (scale * nb->w)) : 0);
        int32_t lightC = ambient + ((nc->w > 0) ? ((lx * nc->x + ly * nc->y + lz * nc->z) / (scale * nc->w)) : 0);

        m->face_colour_a[f] = get_shaded_colour(col, lightA, m->face_render_type ? m->face_render_type[f] : 0);
        m->face_colour_b[f] = get_shaded_colour(col, lightB, m->face_render_type ? m->face_render_type[f] : 0);
        m->face_colour_c[f] = get_shaded_colour(col, lightC, m->face_render_type ? m->face_render_type[f] : 0);
    }

    free(norms);
    return true;
}

void model_calculate_normals(Model *m,int32_t ambient,int32_t contrast,int32_t lx,int32_t ly,int32_t lz) {
    calculate_normals(m,ambient,contrast,lx,ly,lz,false);
}
bool model_calculate_normals_deferred(Model *m,int ambient,int contrast,int lx,int ly,int lz) {
    return calculate_normals(m,ambient,contrast,lx,ly,lz,true);
}

void model_reset_scene_normals(Model *m) {
    if(!m || !m->vertex_normals_original) return;
    memcpy(m->vertex_normals,m->vertex_normals_original,m->num_points*sizeof(ModelNormal));
    memcpy(m->face_render_type,m->face_render_type_original,m->num_faces*sizeof(int32_t));
}

bool model_merge_normals(Model *a,Model *b,int dx,int dy,int dz,bool hide_faces) {
    if(!a || !b || !a->vertex_normals_original || !b->vertex_normals_original) return true;
    unsigned char *marks=hide_faces ? calloc((size_t)a->num_points+b->num_points,1):NULL;
    if(hide_faces && !marks) return false;
    int matches=0;
    for(int i=0;i<a->num_points;i++) {
        ModelNormal *original=&a->vertex_normals_original[i];
        if(!original->w) continue;
        int x=a->point_x[i]-dx,y=a->point_y[i]-dy,z=a->point_z[i]-dz;
        for(int j=0;j<b->num_points;j++) {
            ModelNormal *other=&b->vertex_normals_original[j];
            if(!other->w || x!=b->point_x[j] || y!=b->point_y[j] || z!=b->point_z[j]) continue;
            ModelNormal *an=&a->vertex_normals[i],*bn=&b->vertex_normals[j];
            an->x+=other->x;an->y+=other->y;an->z+=other->z;an->w+=other->w;
            bn->x+=original->x;bn->y+=original->y;bn->z+=original->z;bn->w+=original->w;
            matches++;
            if(marks) {marks[i]=1;marks[a->num_points+j]=1;}
        }
    }
    if(hide_faces && matches>=3) for(int part=0;part<2;part++) {
        Model *m=part ? b:a;unsigned char *matched=marks+(part ? a->num_points:0);
        for(int f=0;f<m->num_faces;f++) if(matched[m->face_vertex_a[f]] && matched[m->face_vertex_b[f]] && matched[m->face_vertex_c[f]])
            m->face_render_type[f]=255;
    }
    free(marks);return true;
}

void model_apply_scene_lighting(Model *m,int ambient,int scale,int lx,int ly,int lz) {
    if(!m || !m->vertex_normals_original) return;
    for(int f=0;f<m->num_faces;f++) {
        int type=m->face_render_type[f];if(type&1) continue;
        int vertices[3]={m->face_vertex_a[f],m->face_vertex_b[f],m->face_vertex_c[f]};
        int32_t *colours[3]={m->face_colour_a,m->face_colour_b,m->face_colour_c};
        for(int k=0;k<3;k++) {
            ModelNormal *n=&m->vertex_normals[vertices[k]];
            int light=ambient+(n->w ? (lx*n->x+ly*n->y+lz*n->z)/(scale*n->w):0);
            colours[k][f]=get_shaded_colour(m->face_colour[f],light,type);
        }
    }
}

/* Bundled revision-225 Model.draw: depth buckets are stable, then the 12
 * cache priority groups are emitted with 10/11 interleaved at the average
 * depths of 1/2, 3/4 and 6/8. No model IDs or geometric offsets are involved. */
static void model_order_faces(const Model *m,int *order,int *depth,int count) {
    for(int i=1;i<count;i++) {
        int face=order[i],d=depth[i],j=i-1;
        while(j>=0 && depth[j]<d) {order[j+1]=order[j];depth[j+1]=depth[j];j--;}
        order[j+1]=face;depth[j+1]=d;
    }
    if(!m->face_priority) return;
    int counts[12]={0},offsets[13]={0},cursor[12];
    int faces[4096],depths[4096];int64_t sums[10]={0};
    for(int i=0;i<count;i++) {
        int priority=m->face_priority[order[i]];
        if(priority<0 || priority>=12) return; // malformed cache cannot index outside the groups
        counts[priority]++;
        if(priority<10) sums[priority]+=depth[i];
    }
    for(int p=0;p<12;p++) {offsets[p+1]=offsets[p]+counts[p];cursor[p]=offsets[p];}
    for(int i=0;i<count;i++) {
        int slot=cursor[m->face_priority[order[i]]]++;
        faces[slot]=order[i];depths[slot]=depth[i];
    }
    int average12=counts[1]+counts[2] ? (int)((sums[1]+sums[2])/(counts[1]+counts[2])):0;
    int average34=counts[3]+counts[4] ? (int)((sums[3]+sums[4])/(counts[3]+counts[4])):0;
    int average68=counts[6]+counts[8] ? (int)((sums[6]+sums[8])/(counts[6]+counts[8])):0;
    int special=offsets[10],end=offsets[12],out=0;
    for(int p=0;p<10;p++) {
        int threshold=p==0 ? average12:p==3 ? average34:average68;
        if(p==0 || p==3 || p==5)
            while(special<end && depths[special]>threshold) order[out++]=faces[special++];
        for(int i=offsets[p];i<offsets[p+1];i++) order[out++]=faces[i];
    }
    while(special<end) order[out++]=faces[special++];
}

void model_draw(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz) {
    if (!m || m->num_points > 4096) return;

    double sinM=pix3d_sin_table[yaw&2047]/65536.0;
    double cosM=pix3d_cos_table[yaw&2047]/65536.0;
    for(int32_t v=0;v<m->num_points;v++) {
        double x=m->point_x[v],y=m->point_y[v],z=m->point_z[v];
        if(yaw!=0) {double tx=z*sinM+x*cosM;z=z*cosM-x*sinM;x=tx;}
        // Rounding each camera-space vertex warps an otherwise coplanar mesh,
        // making its cache-priority details alternately pass/fail shared depth.
        g_camera[v]=pix3d_camera_vertex(x+dx,y+dy,z+dz,eye_pitch,eye_yaw);
        if(g_camera[v].z<50) {g_screen_z[v]=-1;g_screen_x[v]=g_screen_y[v]=-5000;}
        else {
            g_screen_z[v]=(int32_t)g_camera[v].z;
            g_screen_x[v]=(int32_t)(pix3d_center_x+g_camera[v].x*512/g_camera[v].z);
            g_screen_y[v]=(int32_t)(pix3d_center_y+g_camera[v].y*512/g_camera[v].z);
        }
    }

    int32_t face_order[4096];
    int32_t face_depth[4096];
    int32_t visible_faces = 0;

    for (int32_t f = 0; f < m->num_faces && visible_faces < 4096; f++) {
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        if (a<0 || b<0 || c<0 || a>=m->num_points || b>=m->num_points || c>=m->num_points) continue;
        if (m->face_render_type && m->face_render_type[f] == 255) continue;
        bool near_clip = g_screen_z[a]<=0 || g_screen_z[b]<=0 || g_screen_z[c]<=0;

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

        if (!near_clip && (int64_t)dxAB * dyCB - (int64_t)dyAB * dxCB <= 0) {
            continue;
        }

        face_order[visible_faces] = f;
        face_depth[visible_faces] = (g_screen_z[a] + g_screen_z[b] + g_screen_z[c]) / 3;
        visible_faces++;
    }

    model_order_faces(m,face_order,face_depth,visible_faces);

    // Rasterize sorted triangles with full 16-bit HSL << 8
    for (int32_t i = 0; i < visible_faces; i++) {
        int32_t f = face_order[i];
        int32_t a = m->face_vertex_a[f];
        int32_t b = m->face_vertex_b[f];
        int32_t c = m->face_vertex_c[f];

        int type=m->face_render_type ? m->face_render_type[f] : 0;
        Pix3DVertex va=g_camera[a], vb=g_camera[b], vc=g_camera[c];
        va.shade=m->face_colour_a ? m->face_colour_a[f] : m->face_colour[f];
        vb.shade=(type & 1) ? va.shade : (m->face_colour_b ? m->face_colour_b[f] : va.shade);
        vc.shade=(type & 1) ? va.shade : (m->face_colour_c ? m->face_colour_c[f] : va.shade);
        int texture=-1, axis=type>>2;
        if ((type & 2) && axis<m->num_t && m->texture_a) {
            int ta=m->texture_a[axis], tb=m->texture_b[axis], tc=m->texture_c[axis];
            if(ta>=0 && tb>=0 && tc>=0 && ta<m->num_points && tb<m->num_points && tc<m->num_points) {
                texture=m->face_colour[f];
                pix3d_map_texture(&va,g_camera[ta],g_camera[tb],g_camera[tc]);
                pix3d_map_texture(&vb,g_camera[ta],g_camera[tb],g_camera[tc]);
                pix3d_map_texture(&vc,g_camera[ta],g_camera[tb],g_camera[tc]);
            }
        }
        pix3d_scene_triangle(va,vb,vc,texture,m->face_alpha ? m->face_alpha[f] : 0);
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

        if (m->face_render_type && m->face_render_type[f]==255) continue;
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

static bool preview225;
void model_use_preview225(bool enabled){preview225=enabled;}
void model_render_obj(Model *m, int32_t pitch, int32_t yaw, int32_t roll, int32_t eye_pitch, int32_t eye_x, int32_t eye_y, int32_t eye_z) {
    if (!m) return;
    if(preview225){(void)model_render_simple225(m,pitch,yaw,roll,eye_pitch,eye_x,eye_y,eye_z);return;}

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

    model_order_faces(m,face_order,face_depth,visible_faces);

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

/* Explicit Java int operations for drawSimple projection and depth buckets. */
static int32_t m225_int(uint32_t v) { return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v); }
static int32_t m225_add(int32_t a,int32_t b) { return m225_int((uint32_t)a+(uint32_t)b); }
static int32_t m225_sub(int32_t a,int32_t b) { return m225_int((uint32_t)a-(uint32_t)b); }
static int32_t m225_mul(int32_t a,int32_t b) { return m225_int((uint32_t)a*(uint32_t)b); }
static int32_t m225_shr16(int32_t a) {
    return m225_int(((uint32_t)a>>16)|(a<0?UINT32_C(0xffff0000):0));
}
static int32_t m225_sum(int32_t a,int32_t b,int32_t c,int32_t d) {
    return m225_shr16(m225_add(m225_mul(a,b),m225_mul(c,d)));
}
static int32_t m225_diff(int32_t a,int32_t b,int32_t c,int32_t d) {
    return m225_shr16(m225_sub(m225_mul(a,b),m225_mul(c,d)));
}
static int32_t m225_project(int32_t coordinate,int32_t depth) {
    int32_t numerator=m225_int((uint32_t)coordinate<<9);
    return numerator==INT32_MIN && depth==-1?INT32_MIN:numerator/depth;
}

/* Original Model's projection and clipping scratch survives calls. In
 * particular, a completely hidden clipped face can inspect the previous
 * clipped polygon before deciding whether to change Pix3D.hclip. */
typedef struct { int32_t sx[4096],sy[4096],depth[4096],vx[4096],vy[4096],vz[4096]; } Model225Projection;
static Model225Projection g225;
static int32_t g225_clip_x[10],g225_clip_y[10],g225_clip_colour[10];

bool model_render_simple225(const Model *m,int32_t pitch,int32_t yaw,int32_t roll,
    int32_t eye_pitch,int32_t eye_x,int32_t eye_y,int32_t eye_z) {
    if(!m || m->num_points<0 || m->num_points>4096 || m->num_faces<0 || m->num_faces>4096 ||
       (m->num_points && (!m->point_x || !m->point_y || !m->point_z)) ||
       (m->num_faces && (!m->face_vertex_a || !m->face_vertex_b || !m->face_vertex_c ||
        !m->face_colour || !m->face_colour_a || !m->face_colour_b || !m->face_colour_c)) || !pix3d_scanline) return false;
    int32_t sx[4096],sy[4096],depth[4096],vx[4096],vy[4096],vz[4096];
    int32_t order[4096],face_depth[4096];
    const uint32_t *texels[50]={0}; bool opaque[50]={false};
    int32_t sp=pix3d_sin_table[pitch&2047],cp=pix3d_cos_table[pitch&2047];
    int32_t sw=pix3d_sin_table[yaw&2047],cw=pix3d_cos_table[yaw&2047];
    int32_t sr=pix3d_sin_table[roll&2047],cr=pix3d_cos_table[roll&2047];
    int32_t se=pix3d_sin_table[eye_pitch&2047],ce=pix3d_cos_table[eye_pitch&2047];
    int32_t origin=m225_sum(eye_y,se,eye_z,ce);
    int32_t radius_squared=0,max_y=0,min_y=0;
    for(int v=0;v<m->num_points;v++) {
        int32_t x=m->point_x[v],y=m->point_y[v],z=m->point_z[v];
        int32_t r=m225_add(m225_mul(x,x),m225_mul(z,z));
        if(r>radius_squared) radius_squared=r;
        int32_t negative_y=m225_sub(0,y); if(negative_y>max_y) max_y=negative_y;
        if(y>min_y) min_y=y;
        if(roll) { int32_t t=m225_sum(y,sr,x,cr);y=m225_diff(y,cr,x,sr);x=t; }
        if(pitch) { int32_t t=m225_diff(y,cp,z,sp);z=m225_sum(y,sp,z,cp);y=t; }
        if(yaw) { int32_t t=m225_sum(z,sw,x,cw);z=m225_diff(z,cw,x,sw);x=t; }
        x=m225_add(x,eye_x);y=m225_add(y,eye_y);z=m225_add(z,eye_z);
        int32_t t=m225_diff(y,ce,z,se);z=m225_sum(y,se,z,ce);y=t;
        if(z==0) return false;
        vx[v]=x;vy[v]=y;vz[v]=z;depth[v]=m225_sub(z,origin);
        sx[v]=m225_add(pix3d_center_x,m225_project(x,z));
        sy[v]=m225_add(pix3d_center_y,m225_project(y,z));
        g225.sx[v]=sx[v];g225.sy[v]=sy[v];g225.depth[v]=depth[v];
        if(m->num_t>0) {g225.vx[v]=x;g225.vy[v]=y;g225.vz[v]=z;}
    }
    int32_t radius=(int32_t)(sqrt((double)radius_squared)+0.99);
    int32_t bound=m225_add(m225_mul(radius,radius),m225_mul(max_y,max_y));
    if(bound<0) return false;
    int32_t min_depth=(int32_t)(sqrt((double)bound)+0.99);
    bound=m225_add(m225_mul(radius,radius),m225_mul(min_y,min_y));
    if(bound<0) return false;
    int32_t max_depth=min_depth+(int32_t)(sqrt((double)bound)+0.99);
    int count=0;
    for(int f=0;f<m->num_faces;f++) {
        int type=m->face_render_type?m->face_render_type[f]:0;
        if(type==-1 || type==255) continue;
        int a=m->face_vertex_a[f],b=m->face_vertex_b[f],c=m->face_vertex_c[f];
        if(a<0 || b<0 || c<0 || a>=m->num_points || b>=m->num_points || c>=m->num_points ||
           (m->face_priority && (m->face_priority[f]<0 || m->face_priority[f]>11))) return false;
        if(type&2) {
            int axis=type>>2,id=m->face_colour[f];
            if(axis<0 || axis>=m->num_t || id<0 || id>=50 || !m->texture_a || !m->texture_b || !m->texture_c) return false;
            int ta=m->texture_a[axis],tb=m->texture_b[axis],tc=m->texture_c[axis];
            if(ta<0 || tb<0 || tc<0 || ta>=m->num_points || tb>=m->num_points || tc>=m->num_points) return false;
            if(!texels[id]) texels[id]=pix3d225_texture_cache_get(id,&opaque[id]);
            if(!texels[id]) return false;
        }
        int32_t cross=m225_sub(m225_mul(m225_sub(sx[a],sx[b]),m225_sub(sy[c],sy[b])),
                              m225_mul(m225_sub(sy[a],sy[b]),m225_sub(sx[c],sx[b])));
        if(cross<=0) continue;
        int32_t bucket=m225_add(m225_add(m225_add(depth[a],depth[b]),depth[c])/3,min_depth);
        // Original draw traverses only [0,maxDepth), even when drawSimple's
        // pitch/roll puts visible faces beyond that asymmetric cylinder.
        if(bucket<0) return false;
        if(bucket>=max_depth) continue;
        order[count]=f;
        face_depth[count]=bucket;
        count++;
    }
    model_order_faces(m,order,face_depth,count);
    for(int i=0;i<count;i++) {
        int f=order[i],a=m->face_vertex_a[f],b=m->face_vertex_b[f],c=m->face_vertex_c[f];
        int type=m->face_render_type?m->face_render_type[f]:0;
        int ca=m->face_colour_a[f],cb=m->face_colour_b[f],cc=m->face_colour_c[f];
        pix3d_set_hclip(sx[a]<0 || sx[b]<0 || sx[c]<0 || sx[a]>pix2d_safe_width || sx[b]>pix2d_safe_width || sx[c]>pix2d_safe_width);
        pix3d_set_trans(m->face_alpha?m->face_alpha[f]:0);
        switch(type&3) {
        case 0: pix3d225_gouraud_triangle(sx[a],sx[b],sx[c],sy[a],sy[b],sy[c],ca,cb,cc);break;
        case 1: pix3d225_flat_triangle(sx[a],sx[b],sx[c],sy[a],sy[b],sy[c],(uint32_t)pix3d_colour_table[ca]);break;
        default: {
            int axis=type>>2,ta=m->texture_a[axis],tb=m->texture_b[axis],tc=m->texture_c[axis];
            if(type&1) cb=cc=ca;
            int32_t args[18]={sy[a],sy[b],sy[c],sx[a],sx[b],sx[c],ca,cb,cc,
                vx[ta],vx[tb],vx[tc],vy[ta],vy[tb],vy[tc],vz[ta],vz[tb],vz[tc]};
            pix3d225_texture_triangle(texels[m->face_colour[f]],opaque[m->face_colour[f]],args);
            break;
        }
        }
    }
    return true;
}

static int32_t m225_cross(int32_t ax,int32_t bx,int32_t cx,int32_t ay,int32_t by,int32_t cy) {
    return m225_sub(m225_mul(m225_sub(ax,bx),m225_sub(cy,by)),m225_mul(m225_sub(ay,by),m225_sub(cx,bx)));
}

/* Model.drawFace's four primitive branches. Alpha and clipping are set by
 * the caller: drawNearClippedFace deliberately inherits the existing alpha. */
static void m225_emit(const Model *m,int f,const int32_t x[3],const int32_t y[3],
    const int32_t shade[3],const uint32_t *const texels[50],const bool opaque[50]) {
    int type=m->face_render_type?m->face_render_type[f]:0;
    int ca=shade[0],cb=shade[1],cc=shade[2];
    switch(type&3) {
    case 0: pix3d225_gouraud_triangle(x[0],x[1],x[2],y[0],y[1],y[2],ca,cb,cc);break;
    case 1: pix3d225_flat_triangle(x[0],x[1],x[2],y[0],y[1],y[2],(uint32_t)pix3d_colour_table[m->face_colour_a[f]]);break;
    default: {
        int axis=type>>2,ta=m->texture_a[axis],tb=m->texture_b[axis],tc=m->texture_c[axis];
        if(type&1) ca=cb=cc=m->face_colour_a[f];
        int32_t args[18]={y[0],y[1],y[2],x[0],x[1],x[2],ca,cb,cc,
            g225.vx[ta],g225.vx[tb],g225.vx[tc],g225.vy[ta],g225.vy[tb],g225.vy[tc],g225.vz[ta],g225.vz[tb],g225.vz[tc]};
        pix3d225_texture_triangle(texels[m->face_colour[f]],opaque[m->face_colour[f]],args);break;
    }
    }
}

static bool m225_clip_face(const Model *m,int f,const uint32_t *const texels[50],const bool opaque[50]) {
    int vertices[3]={m->face_vertex_a[f],m->face_vertex_b[f],m->face_vertex_c[f]};
    int32_t colours[3]={m->face_colour_a[f],m->face_colour_b[f],m->face_colour_c[f]};
    int count=0;
    for(int k=0;k<3;k++) {
        int v=vertices[k];
        if(g225.vz[v]>=50) {
            g225_clip_x[count]=g225.sx[v];g225_clip_y[count]=g225.sy[v];g225_clip_colour[count++]=colours[k];
        } else for(int side=0;side<2;side++) {
            // Original order: previous edge then next edge at each hidden vertex.
            int j=(k+(side==0?2:1))%3,w=vertices[j];
            if(g225.vz[w]<50) continue;
            int32_t distance=m225_sub(g225.vz[w],g225.vz[v]);
            if(distance<=0 || distance>=2048) return false;
            int32_t t=m225_mul(m225_sub(50,g225.vz[v]),pix3d_div_table2[distance]);
            int32_t x=m225_add(g225.vx[v],m225_shr16(m225_mul(m225_sub(g225.vx[w],g225.vx[v]),t)));
            int32_t y=m225_add(g225.vy[v],m225_shr16(m225_mul(m225_sub(g225.vy[w],g225.vy[v]),t)));
            g225_clip_x[count]=m225_add(pix3d_center_x,m225_project(x,50));
            g225_clip_y[count]=m225_add(pix3d_center_y,m225_project(y,50));
            g225_clip_colour[count++]=m225_add(colours[k],m225_shr16(m225_mul(m225_sub(colours[j],colours[k]),t)));
        }
    }
    if(m225_cross(g225_clip_x[0],g225_clip_x[1],g225_clip_x[2],g225_clip_y[0],g225_clip_y[1],g225_clip_y[2])<=0) return true;
    pix3d_set_hclip(false);
    if(count!=3 && count!=4) return true;
    for(int k=0;k<count;k++) if(g225_clip_x[k]<0 || g225_clip_x[k]>pix2d_safe_width) pix3d_set_hclip(true);
    m225_emit(m,f,g225_clip_x,g225_clip_y,g225_clip_colour,texels,opaque);
    if(count==4) {
        int32_t x[3]={g225_clip_x[0],g225_clip_x[2],g225_clip_x[3]};
        int32_t y[3]={g225_clip_y[0],g225_clip_y[2],g225_clip_y[3]};
        int32_t c[3]={g225_clip_colour[0],g225_clip_colour[2],g225_clip_colour[3]};
        m225_emit(m,f,x,y,c,texels,opaque);
    }
    return true;
}

/* Despite its Java name, pointWithinTriangle is an inclusive face bounding
 * box check. It intentionally precedes backface culling in Model.draw. */
bool model_point_within_triangle225(int32_t mx,int32_t my,int32_t ya,int32_t yb,int32_t yc,
    int32_t xa,int32_t xb,int32_t xc) {
    if(my<ya && my<yb && my<yc)return false;
    if(my>ya && my>yb && my>yc)return false;
    if(mx<xa && mx<xb && mx<xc)return false;
    return mx<=xa || mx<=xb || mx<=xc;
}

static bool m225_world(const Model *m,int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,
    int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick,const ModelBounds225 *bounds,bool draw) {
    /* Real cache models may have vertices but no faces/face arrays. Still
     * transform their vertices: original projection scratch survives draws. */
    if(!m || m->num_points<0 || m->num_points>4096 || m->num_faces<0 || m->num_faces>4096 ||
       (m->num_points && (!m->point_x || !m->point_y || !m->point_z)) ||
       (m->num_faces && (!m->face_vertex_a || !m->face_vertex_b || !m->face_vertex_c ||
        !m->face_colour || !m->face_colour_a || !m->face_colour_b || !m->face_colour_c)) || !pix3d_scanline) return false;
    int32_t radius,max_y=0,min_depth,max_depth;
    if(bounds){
        radius=bounds->radius;max_y=bounds->max_y;min_depth=bounds->min_depth;max_depth=bounds->max_depth;
        if(radius<0 || max_y<0 || min_depth<0 || max_depth<min_depth)return false;
    }else{
        int32_t radius_squared=0,min_y=0;
        for(int v=0;v<m->num_points;v++) {
            int32_t x=m->point_x[v],y=m->point_y[v],z=m->point_z[v];
            int32_t r=m225_add(m225_mul(x,x),m225_mul(z,z));
            if(r>radius_squared)radius_squared=r;
            int32_t ny=m225_sub(0,y);if(ny>max_y)max_y=ny;if(y>min_y)min_y=y;
        }
        radius=(int32_t)(sqrt((double)radius_squared)+0.99);
        int32_t bound=m225_add(m225_mul(radius,radius),m225_mul(max_y,max_y));if(bound<0)return false;
        min_depth=(int32_t)(sqrt((double)bound)+0.99);
        bound=m225_add(m225_mul(radius,radius),m225_mul(min_y,min_y));if(bound<0)return false;
        max_depth=min_depth+(int32_t)(sqrt((double)bound)+0.99);
    }
    int32_t sp=pix3d_sin_table[eye_pitch&2047],cp=pix3d_cos_table[eye_pitch&2047];
    int32_t sw=pix3d_sin_table[eye_yaw&2047],cw=pix3d_cos_table[eye_yaw&2047];
    int32_t base_z=m225_diff(dz,cw,dx,sw),origin=m225_sum(dy,sp,base_z,cp);
    int32_t radius_z=m225_shr16(m225_mul(radius,cp)),far_z=m225_add(origin,radius_z);
    if(far_z<=50 || origin>=3500) return true;
    int32_t base_x=m225_sum(dz,sw,dx,cw);
    if(m225_project(m225_sub(base_x,radius),far_z)>=pix2d_center_x) return true;
    if(m225_project(m225_add(base_x,radius),far_z)<=-pix2d_center_x) return true;
    int32_t base_y=m225_diff(dy,cp,base_z,sp),radius_y=m225_shr16(m225_mul(radius,sp));
    if(m225_project(m225_add(base_y,radius_y),far_z)<=-pix2d_center_y) return true;
    int32_t height=m225_add(radius_y,m225_shr16(m225_mul(max_y,cp)));
    if(m225_project(m225_sub(base_y,height),far_z)>=pix2d_center_y) return true;
    int32_t near_extent=m225_add(radius_z,m225_shr16(m225_mul(max_y,sp)));
    bool near=m225_sub(origin,near_extent)<=50;
    bool check_faces=false;
    if(pick && pick->bitset>0 && pick->check_hover) {
        int32_t near_z=m225_sub(origin,radius_z);if(near_z<=50)near_z=50;
        int32_t left=m225_project(m225_sub(base_x,radius),base_x>0?far_z:near_z);
        int32_t right=m225_project(m225_add(base_x,radius),base_x>0?near_z:far_z);
        int32_t top=m225_project(m225_sub(base_y,height),base_y>0?far_z:near_z);
        int32_t bottom=m225_project(m225_add(base_y,radius_y),base_y>0?near_z:far_z);
        int32_t mx=m225_sub(pick->mouse_x,pix3d_center_x),my=m225_sub(pick->mouse_y,pix3d_center_y);
        if(mx>left && mx<right && my>top && my<bottom) {
            if(pick->pickable)pick->picked=true;
            else check_faces=true;
        }
    }
    int32_t sm=pix3d_sin_table[yaw&2047],cm=pix3d_cos_table[yaw&2047];
    for(int v=0;v<m->num_points;v++) {
        int32_t x=m->point_x[v],y=m->point_y[v],z=m->point_z[v];
        if(yaw) {int32_t t=m225_sum(z,sm,x,cm);z=m225_diff(z,cm,x,sm);x=t;}
        x=m225_add(x,dx);y=m225_add(y,dy);z=m225_add(z,dz);
        int32_t t=m225_sum(z,sw,x,cw);z=m225_diff(z,cw,x,sw);x=t;
        t=m225_diff(y,cp,z,sp);z=m225_sum(y,sp,z,cp);y=t;
        g225.depth[v]=m225_sub(z,origin);
        if(z>=50) {g225.sx[v]=m225_add(pix3d_center_x,m225_project(x,z));g225.sy[v]=m225_add(pix3d_center_y,m225_project(y,z));}
        else {g225.sx[v]=-5000;near=true;}
        if(near || m->num_t>0) {g225.vx[v]=x;g225.vy[v]=y;g225.vz[v]=z;}
    }
    int32_t order[4096],face_depth[4096];bool clipped[4096];int count=0;
    const uint32_t *texels[50]={0};bool opaque[50]={false};
    for(int f=0;f<m->num_faces;f++) {
        int type=m->face_render_type?m->face_render_type[f]:0;if(type==-1 || type==255)continue;
        int a=m->face_vertex_a[f],b=m->face_vertex_b[f],c=m->face_vertex_c[f];
        if(a<0 || b<0 || c<0 || a>=m->num_points || b>=m->num_points || c>=m->num_points ||
           (m->face_priority && (m->face_priority[f]<0 || m->face_priority[f]>11))) return false;
        clipped[f]=near && (g225.sx[a]==-5000 || g225.sx[b]==-5000 || g225.sx[c]==-5000);
        if(!clipped[f] && check_faces && model_point_within_triangle225(pick->mouse_x,pick->mouse_y,
            g225.sy[a],g225.sy[b],g225.sy[c],g225.sx[a],g225.sx[b],g225.sx[c])) {
            pick->picked=true;check_faces=false;
        }
        if(!clipped[f] && m225_cross(g225.sx[a],g225.sx[b],g225.sx[c],g225.sy[a],g225.sy[b],g225.sy[c])<=0) continue;
        int32_t bucket=m225_add(m225_add(m225_add(g225.depth[a],g225.depth[b]),g225.depth[c])/3,min_depth);
        if(bucket<0)return false;
        if(bucket>=max_depth)continue;
        if((type&2) && draw) {
            int axis=type>>2,id=m->face_colour[f];
            if(axis<0 || axis>=m->num_t || id<0 || id>=50 || !m->texture_a || !m->texture_b || !m->texture_c)return false;
            int ta=m->texture_a[axis],tb=m->texture_b[axis],tc=m->texture_c[axis];
            if(ta<0 || tb<0 || tc<0 || ta>=m->num_points || tb>=m->num_points || tc>=m->num_points)return false;
            if(!texels[id])texels[id]=pix3d225_texture_cache_get(id,&opaque[id]);
            if(!texels[id])return false;
        }
        order[count]=f;face_depth[count++]=bucket;
    }
    if(!draw)return true;
    model_order_faces(m,order,face_depth,count);
    for(int i=0;i<count;i++) {
        int f=order[i];
        if(clipped[f]) {if(!m225_clip_face(m,f,texels,opaque))return false;continue;}
        int a=m->face_vertex_a[f],b=m->face_vertex_b[f],c=m->face_vertex_c[f];
        int32_t x[3]={g225.sx[a],g225.sx[b],g225.sx[c]},y[3]={g225.sy[a],g225.sy[b],g225.sy[c]};
        int32_t shade[3]={m->face_colour_a[f],m->face_colour_b[f],m->face_colour_c[f]};
        pix3d_set_hclip(x[0]<0 || x[1]<0 || x[2]<0 || x[0]>pix2d_safe_width || x[1]>pix2d_safe_width || x[2]>pix2d_safe_width);
        pix3d_set_trans(m->face_alpha?m->face_alpha[f]:0);
        m225_emit(m,f,x,y,shade,texels,opaque);
    }
    return true;
}

bool model_render_world225(const Model *m,int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,
    int32_t dx,int32_t dy,int32_t dz) {
    return m225_world(m,yaw,eye_pitch,eye_yaw,dx,dy,dz,NULL,NULL,true);
}
bool model_render_world_bounds225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz) {
    if(!bounds)return false;
    return m225_world(m,yaw,eye_pitch,eye_yaw,dx,dy,dz,NULL,bounds,true);
}

bool model_render_world_pick225(const Model *m,int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,
    int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick) {
    if(pick)pick->picked=false;
    return m225_world(m,yaw,eye_pitch,eye_yaw,dx,dy,dz,pick,NULL,true);
}

bool model_render_world_bounds_pick225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick) {
    if(pick)pick->picked=false;
    if(!bounds)return false;
    return m225_world(m,yaw,eye_pitch,eye_yaw,dx,dy,dz,pick,bounds,true);
}
bool model_probe_world_bounds225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick){
    if(!pick || !bounds)return false;
    pick->picked=false;
    Model225Projection saved=g225;
    bool ok=m225_world(m,yaw,eye_pitch,eye_yaw,dx,dy,dz,pick,bounds,false);g225=saved;return ok;
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

    dst->allocation_arena=arena;
    dst->num_points = src->num_points;
    dst->num_faces = src->num_faces;
    dst->num_t = src->num_t;
    if (src->num_t) {
        size_t size=src->num_t*sizeof(int32_t);
        dst->texture_a=arena ? arena_alloc(arena,size) : malloc(size);
        dst->texture_b=arena ? arena_alloc(arena,size) : malloc(size);
        dst->texture_c=arena ? arena_alloc(arena,size) : malloc(size);
        memcpy(dst->texture_a,src->texture_a,size);
        memcpy(dst->texture_b,src->texture_b,size);
        memcpy(dst->texture_c,src->texture_c,size);
    }
    dst->priority = src->priority;
    dst->min_depth = src->min_depth;
    dst->max_depth = src->max_depth;
    dst->obj_raise = src->obj_raise;
    if(src->vertex_normals_original) {
        size_t ns=src->num_points*sizeof(ModelNormal),fs=src->num_faces*sizeof(int32_t);
        dst->vertex_normals=arena ? arena_alloc(arena,ns):malloc(ns);
        dst->vertex_normals_original=arena ? arena_alloc(arena,ns):malloc(ns);
        dst->face_render_type_original=arena ? arena_alloc(arena,fs):malloc(fs);
        if(!dst->vertex_normals || !dst->vertex_normals_original || !dst->face_render_type_original) {model_free_unshared(dst);return NULL;}
        memcpy(dst->vertex_normals,src->vertex_normals,ns);
        memcpy(dst->vertex_normals_original,src->vertex_normals_original,ns);
        memcpy(dst->face_render_type_original,src->face_render_type_original,fs);
    }

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
        dst->num_label_vertices = src->num_label_vertices;
        {
            size_t count=src->num_label_vertices;
            dst->label_vertices=arena ? arena_alloc_zero(arena,count*sizeof(int32_t*)):calloc(count,sizeof(int32_t*));
            dst->label_vertex_counts=arena ? arena_alloc(arena,count*sizeof(int32_t)):malloc(count*sizeof(int32_t));
            memcpy(dst->label_vertex_counts,src->label_vertex_counts,count*sizeof(int32_t));
            for(size_t i=0;i<count;i++) if(src->label_vertex_counts[i]) {
                size_t bytes=src->label_vertex_counts[i]*sizeof(int32_t);dst->label_vertices[i]=arena ? arena_alloc(arena,bytes):malloc(bytes);memcpy(dst->label_vertices[i],src->label_vertices[i],bytes);
            }
        }
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
    int32_t total_t = 0;
    bool has_alpha=false,has_face_labels=false;

    for (int32_t i = 0; i < count; i++) {
        if (models[i]) {
            total_points += models[i]->num_points;
            total_faces += models[i]->num_faces;
            total_t += models[i]->num_t;
            has_alpha|=models[i]->face_alpha!=NULL;has_face_labels|=models[i]->face_label!=NULL;
        }
    }
    if (total_faces == 0) return NULL;

    Model *m = (Model *)arena_alloc(arena, sizeof(Model));
    memset(m, 0, sizeof(Model));
    m->allocation_arena=arena;

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
    if(has_alpha)m->face_alpha = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));
    if(has_face_labels)m->face_label = (int32_t *)arena_alloc_zero(arena, total_faces * sizeof(int32_t));

    m->texture_a=arena_alloc(arena,total_t*sizeof(int32_t));
    m->texture_b=arena_alloc(arena,total_t*sizeof(int32_t));
    m->texture_c=arena_alloc(arena,total_t*sizeof(int32_t));
    m->num_points = 0;
    m->num_faces = 0;

    for (int32_t i = 0; i < count; i++) {
        Model *src = models[i];
        if (!src) continue;
        for (int32_t f = 0; f < src->num_faces; f++) {
            int out_f = m->num_faces++;
            m->face_colour[out_f] = src->face_colour[f];
            if (src->face_render_type) m->face_render_type[out_f] = src->face_render_type[f];
            m->face_priority[out_f] = src->face_priority ? src->face_priority[f] : src->priority;
            if (src->face_alpha) m->face_alpha[out_f] = src->face_alpha[f];
            if (src->face_label) m->face_label[out_f] = src->face_label[f];

            m->face_vertex_a[out_f] = add_composite_vertex(m, src, src->face_vertex_a[f]);
            m->face_vertex_b[out_f] = add_composite_vertex(m, src, src->face_vertex_b[f]);
            m->face_vertex_c[out_f] = add_composite_vertex(m, src, src->face_vertex_c[f]);
        }
        for(int t=0;t<src->num_t;t++) {
            m->texture_a[m->num_t]=add_composite_vertex(m,src,src->texture_a[t]);
            m->texture_b[m->num_t]=add_composite_vertex(m,src,src->texture_b[t]);
            m->texture_c[m->num_t]=add_composite_vertex(m,src,src->texture_c[t]);
            m->num_t++;
        }
    }
    return m;
}

Model *model_combine_lit(Model **models,int count,Arena *arena) {
    if(!arena || !models || count<=0) return NULL;
    int points=0,faces=0,textures=0;
    for(int i=0;i<count;i++) if(models[i]) {points+=models[i]->num_points;faces+=models[i]->num_faces;textures+=models[i]->num_t;}
    size_t need=sizeof(Model)+(size_t)points*3*sizeof(int32_t)+(size_t)faces*10*sizeof(int32_t)+(size_t)textures*3*sizeof(int32_t)+256;
    if(need>arena->capacity-arena->offset) return NULL;
    Model *m=arena_alloc_zero(arena,sizeof(*m));m->allocation_arena=arena;
    m->num_points=points;m->num_faces=faces;m->num_t=textures;
    #define ARRAY(field,n) m->field=arena_alloc_zero(arena,(size_t)(n)*sizeof(int32_t))
    ARRAY(point_x,points);ARRAY(point_y,points);ARRAY(point_z,points);
    ARRAY(face_vertex_a,faces);ARRAY(face_vertex_b,faces);ARRAY(face_vertex_c,faces);
    ARRAY(face_colour,faces);ARRAY(face_colour_a,faces);ARRAY(face_colour_b,faces);ARRAY(face_colour_c,faces);
    ARRAY(face_render_type,faces);ARRAY(face_priority,faces);ARRAY(face_alpha,faces);
    ARRAY(texture_a,textures);ARRAY(texture_b,textures);ARRAY(texture_c,textures);
    #undef ARRAY
    int vertex=0,face=0,texture=0;
    for(int i=0;i<count;i++) {Model *src=models[i];if(!src) continue;
        memcpy(m->point_x+vertex,src->point_x,src->num_points*sizeof(int32_t));
        memcpy(m->point_y+vertex,src->point_y,src->num_points*sizeof(int32_t));
        memcpy(m->point_z+vertex,src->point_z,src->num_points*sizeof(int32_t));
        for(int f=0;f<src->num_faces;f++,face++) {
            m->face_vertex_a[face]=src->face_vertex_a[f]+vertex;m->face_vertex_b[face]=src->face_vertex_b[f]+vertex;m->face_vertex_c[face]=src->face_vertex_c[f]+vertex;
            m->face_colour[face]=src->face_colour?src->face_colour[f]:0;
            m->face_colour_a[face]=src->face_colour_a?src->face_colour_a[f]:0;m->face_colour_b[face]=src->face_colour_b?src->face_colour_b[f]:0;m->face_colour_c[face]=src->face_colour_c?src->face_colour_c[f]:0;
            m->face_priority[face]=src->face_priority?src->face_priority[f]:src->priority;
            m->face_render_type[face]=src->face_render_type?src->face_render_type[f]:0;
            m->face_alpha[face]=src->face_alpha?src->face_alpha[f]:0;
        }
        for(int t=0;t<src->num_t;t++,texture++) {m->texture_a[texture]=src->texture_a[t]+vertex;m->texture_b[texture]=src->texture_b[t]+vertex;m->texture_c[texture]=src->texture_c[t]+vertex;}
        vertex+=src->num_points;
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
    // Scenery copies are heap-backed (loc_get_model passes NULL), while animated
    // entity copies use an arena. Preserve the allocation policy of the caller.
    m->label_vertex_counts = arena ? (int32_t *)arena_alloc(arena, m->num_label_vertices * sizeof(int32_t))
                                   : (int32_t *)malloc(m->num_label_vertices * sizeof(int32_t));
    m->label_vertices = arena ? (int32_t **)arena_alloc(arena, m->num_label_vertices * sizeof(int32_t *))
                             : (int32_t **)malloc(m->num_label_vertices * sizeof(int32_t *));

    for (int i = 0; i <= max_lbl; i++) {
        m->label_vertex_counts[i] = counts[i];
        if (counts[i] > 0) {
            m->label_vertices[i] = arena ? (int32_t *)arena_alloc(arena, counts[i] * sizeof(int32_t))
                                        : (int32_t *)malloc(counts[i] * sizeof(int32_t));
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
    if (!m || !labels) return;

    // Original Model transform type 5 operates on face-label groups, not
    // vertex groups. The old implementation silently omitted alpha animation.
    if(type==5) {
        if(m->face_label && m->face_alpha) for(int group=0;group<label_count;group++)
            for(int face=0;face<m->num_faces;face++) if(m->face_label[face]==labels[group]) {
                int alpha=m->face_alpha[face]+dx*8;
                m->face_alpha[face]=alpha<0?0:alpha>255?255:alpha;
            }
        return;
    }
    if(!m->label_vertices) return;

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
        if(grp<0 || grp>=f1->base->length)continue;
        if (grp == mask_val || f1->base->types[grp] == 0) {
            apply_single_transform(m, f1->base->types[grp], f1->base->labels[grp], f1->base->label_counts[grp], f2->x[i], f2->y[i], f2->z[i]);
        }
    }
}

bool model_share_alpha(Model *destination,Model *source) {
    if(!source->face_alpha || source->allocation_arena) return false;
    if(!source->alpha_storage) {
        int32_t *storage=malloc((source->num_faces+1)*sizeof(int32_t));
        if(!storage) return false;
        storage[0]=1;memcpy(storage+1,source->face_alpha,source->num_faces*sizeof(int32_t));
        free(source->face_alpha);source->alpha_storage=storage;source->face_alpha=storage+1;
    }
    if(destination->allocation_arena) {
        if(!arena_retain_shared(destination->allocation_arena,source->alpha_storage))return false;
    } else {
        if(destination->alpha_storage) {if(--destination->alpha_storage[0]==0)free(destination->alpha_storage);}
        else free(destination->face_alpha);
        source->alpha_storage[0]++;
    }
    destination->alpha_storage=source->alpha_storage;destination->face_alpha=source->face_alpha;
    return true;
}

void model_free_unshared(Model *m) {
    if(!m || m->allocation_arena) return;
    if(m->alpha_storage) {
        if(--m->alpha_storage[0]==0) free(m->alpha_storage);
        m->face_alpha=NULL;
    }
    if(m->label_vertices) for(int i=0;i<m->num_label_vertices;i++) free(m->label_vertices[i]);
    void *owned[]={m->point_x,m->point_y,m->point_z,m->face_vertex_a,m->face_vertex_b,m->face_vertex_c,
        m->texture_a,m->texture_b,m->texture_c,m->face_colour,m->face_colour_a,m->face_colour_b,m->face_colour_c,
        m->face_render_type,m->face_priority,m->face_alpha,m->face_label,m->vertex_label,m->label_vertices,m->label_vertex_counts,
        m->vertex_normals,m->vertex_normals_original,m->face_render_type_original};
    for(size_t i=0;i<sizeof(owned)/sizeof(owned[0]);i++) {
        bool duplicate=false;for(size_t j=0;j<i;j++) if(owned[i]==owned[j]) duplicate=true;
        if(!duplicate) free(owned[i]);
    }
    free(m);
}
