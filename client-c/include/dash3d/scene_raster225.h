#ifndef RS2_SCENE_RASTER225_H
#define RS2_SCENE_RASTER225_H
#include "dash3d/scene_traversal225.h"
#include "dash3d/ground225.h"

/* Geometry is indexed by Ground identity, including bridge identities.
 * Model resolution supplies prepared lighting/bounds by an opaque 32-byte key
 * (snapshot digest or native construction identity).
 * Neither interface supplies callback order. The caller owns all storage. */
typedef struct {
    const TileUnderlay *underlay;
    const TileOverlay *overlay;
} SceneGeometry225;
typedef struct {
    Model *model;
    ModelBounds225 bounds;
    bool pickable; /* Original Model.pickable, not location interactivity. */
} SceneModel225;
typedef const SceneModel225 *(*SceneResolve225)(void *user,const uint8_t digest[32]);
typedef struct {
    bool check_hover;
    int32_t mouse_x,mouse_y;
    int32_t *bitsets;
    size_t count,capacity;
} ScenePicking225;
typedef struct {
    const SceneGeometry225 *geometry;
    size_t geometry_count;
    SceneResolve225 resolve;
    void *resolve_user;
    Ground225 view;
    uint64_t models,underlays,overlays;
    int32_t error; /* 1: inputs; 2: geometry; 3: model; 4: raster;
                    5: observer; 6: traversal/completion; 7: pick capacity. */
    SceneCall225 failed_call;
    ScenePicking225 picking;
} SceneRaster225;

/* Explicit, isolated scene entry point; does not change world3d_render.
 * Bind the surface and prepare the 225 palette/texture cache beforehand.
 * scene->emit, if supplied, observes each entry before drawing. It is restored
 * on return. Hover appends to the caller's list in draw order, without reset
 * or deduplication. Ground input/coordinates use view; the caller performs
 * World3D.click's reset before a new click. On completion view.taking_input
 * mirrors scene->taking_input. A miss/disabled input preserves coordinates. */
bool world3d_render_scene225(SceneTraversal225 *scene,int32_t root_id,SceneRaster225 *raster);
#endif
