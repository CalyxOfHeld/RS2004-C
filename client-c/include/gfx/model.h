#ifndef RS2_MODEL_H
#define RS2_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct {
    Arena *allocation_arena;
    int32_t num_points;
    int32_t num_faces;
    int32_t num_t;

    int32_t *point_x;
    int32_t *point_y;
    int32_t *point_z;

    int32_t *face_vertex_a;
    int32_t *face_vertex_b;
    int32_t *face_vertex_c;

    int32_t *texture_a, *texture_b, *texture_c;

    int32_t *face_colour;
    int32_t *face_colour_a;
    int32_t *face_colour_b;
    int32_t *face_colour_c;

    int32_t *face_render_type;
    int32_t *face_priority;
    int32_t *face_alpha;
    int32_t *face_label;
    int32_t *vertex_label;

    int32_t num_label_vertices;
    int32_t *label_vertex_counts;
    int32_t **label_vertices;

    int32_t priority;
    int32_t min_depth;
    int32_t max_depth;
} Model;

void   model_init_repo(Jagfile *models_jag, Arena *arena);
Model *model_load(int32_t id, Arena *arena);
// Only for an unshared, heap-owned model_load result (never an arena/copy view).
void model_free_unshared(Model *model);
void   model_recolor(Model *m, int32_t src_color, int32_t dst_color);
void   model_translate(Model *m, int32_t y, int32_t x, int32_t z);
Model *model_copy(const Model *src, Arena *arena);
Model *model_create_composite(Model **models, int32_t count, Arena *arena);
void   model_create_label_references(Model *m, Arena *arena);
void   model_apply_transform(Model *m, int frame_id);
void   model_apply_transforms(Model *m, int primary_frame, int secondary_frame, const int *mask);
void   model_calculate_normals(Model *m, int32_t ambient, int32_t contrast, int32_t lx, int32_t ly, int32_t lz);
void   model_draw(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz);
bool   model_draw_pickable(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz, int mouse_x, int mouse_y);
void   model_render_obj(Model *m, int32_t pitch, int32_t yaw, int32_t roll, int32_t eye_pitch, int32_t eye_x, int32_t eye_y, int32_t eye_z);

#endif // RS2_MODEL_H
