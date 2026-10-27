#ifndef RS2_MODEL_H
#define RS2_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct { int32_t x,y,z,w; } ModelNormal;

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
    // Heap-only shared alpha, as used by original LocType's model caches.
    int32_t *alpha_storage;
    int32_t *face_label;
    int32_t *vertex_label;
    // LocType.sharelight: immutable pre-skew sums and scene working sums.
    ModelNormal *vertex_normals, *vertex_normals_original;
    int32_t *face_render_type_original;

    int32_t num_label_vertices;
    int32_t *label_vertex_counts;
    int32_t **label_vertices;

    int32_t priority;
    int32_t min_depth;
    int32_t max_depth;
    int32_t obj_raise; // Original LocType.blockwalk model height for item stacks.
} Model;

void   model_init_repo(Jagfile *models_jag, Arena *arena);
Model *model_load(int32_t id, Arena *arena);
// Heap-owned model arrays; also releases reference-counted scenery alpha.
void model_free_unshared(Model *model);
bool model_share_alpha(Model *destination,Model *source);
void   model_recolor(Model *m, int32_t src_color, int32_t dst_color);
void   model_translate(Model *m, int32_t y, int32_t x, int32_t z);
Model *model_copy(const Model *src, Arena *arena);
Model *model_create_composite(Model **models, int32_t count, Arena *arena);
// Original lit two-model constructor: concatenate vertices and preserve shading.
Model *model_combine_lit(Model **models,int count,Arena *arena);
void   model_create_label_references(Model *m, Arena *arena);
void   model_apply_transform(Model *m, int frame_id);
void   model_apply_transforms(Model *m, int primary_frame, int secondary_frame, const int *mask);
void   model_calculate_normals(Model *m, int32_t ambient, int32_t contrast, int32_t lx, int32_t ly, int32_t lz);
bool   model_calculate_normals_deferred(Model *m,int ambient,int contrast,int lx,int ly,int lz);
void   model_reset_scene_normals(Model *m);
bool   model_merge_normals(Model *a,Model *b,int dx,int dy,int dz,bool hide_faces);
void   model_apply_scene_lighting(Model *m,int ambient,int scale,int lx,int ly,int lz);
void   model_draw(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz);
bool   model_draw_pickable(Model *m, int32_t yaw, int32_t eye_pitch, int32_t eye_yaw, int32_t dx, int32_t dy, int32_t dz, int mouse_x, int mouse_y);
void   model_render_obj(Model *m, int32_t pitch, int32_t yaw, int32_t roll, int32_t eye_pitch, int32_t eye_x, int32_t eye_y, int32_t eye_z);
/* Candidate client opt-in for interface/item drawSimple calls. */
void model_use_preview225(bool enabled);
/* Original drawSimple contract; separate from the installed scene/preview
 * path. Caller prepares the 225 palette/texture cache and bound surface.
 * Returns false for unsupported capacities, invalid indices or zero depth.
 * drawSimple does not perform near-plane clipping (unlike Model.draw). */
bool model_render_simple225(const Model *m,int32_t pitch,int32_t yaw,int32_t roll,
    int32_t eye_pitch,int32_t eye_x,int32_t eye_y,int32_t eye_z);
/* Original world-space draw, with frustum tests and z=50 face clipping.
 * Same palette/cache contract as simple225; hover/picking is not performed.
 * A valid model culled by the frustum returns true without drawing. */
bool model_render_world225(const Model *m,int32_t yaw,int32_t eye_pitch,
    int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz);
typedef struct {int32_t radius,max_y,min_depth,max_depth;} ModelBounds225;
/* Original scene models carry bounds from cylinder/AABB/Y calculations.
 * This entry uses the supplied original bounds without recomputing them. */
bool model_render_world_bounds225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz);
typedef struct {
    bool check_hover;
    bool pickable; // Original Model.pickable: coarse bounds alone select it.
    int32_t mouse_x,mouse_y,bitset;
    bool picked; // Cleared on entry; at most one selection per model draw.
} ModelPick225;
/* Rendering plus original hover selection. Caller appends bitset to its
 * ordered pick list when picked is true; no global list is owned here. */
bool model_render_world_pick225(const Model *m,int32_t yaw,int32_t eye_pitch,
    int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick);
/* Same hover contract using the model's prepared cylinder/AABB/Y bounds. */
bool model_render_world_bounds_pick225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick);
/* Input-script pointer resolution: same selection, no pixels or scratch change. */
bool model_probe_world_bounds225(const Model *m,const ModelBounds225 *bounds,
    int32_t yaw,int32_t eye_pitch,int32_t eye_yaw,int32_t dx,int32_t dy,int32_t dz,ModelPick225 *pick);
/* Original Model.pointWithinTriangle is an inclusive face bounding box. */
bool model_point_within_triangle225(int32_t mx,int32_t my,int32_t ya,int32_t yb,int32_t yc,
    int32_t xa,int32_t xb,int32_t xc);

#endif // RS2_MODEL_H
