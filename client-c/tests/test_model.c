#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"

int main(void) {
    printf("[Test Model] Testing 3D Model parsing & lighting against live 2004scape cache...\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    assert(models_jag != NULL);
    printf("  'models' Jagfile loaded successfully!\n");

    model_init_repo(models_jag, &arena);

    // 1. Load Model 0
    Model *m0 = model_load(0, &arena);
    assert(m0 != NULL);
    printf("  Model 0 loaded: points=%d (expected 176), faces=%d (expected 327)\n",
           m0->num_points, m0->num_faces);
    assert(m0->num_points == 176);
    assert(m0->num_faces == 327);

    // Verify vertex 0 & 1 coordinates
    printf("  Vertex 0: (%d, %d, %d) [expected (-40, -10, -16)]\n",
           m0->point_x[0], m0->point_y[0], m0->point_z[0]);
    assert(m0->point_x[0] == -40);
    assert(m0->point_y[0] == -10);
    assert(m0->point_z[0] == -16);

    assert(m0->point_x[1] == -40);
    assert(m0->point_y[1] == -10);
    assert(m0->point_z[1] == -8);

    // Verify face 0 vertices
    printf("  Face 0 vertices: (%d, %d, %d) [expected (0, 1, 2)]\n",
           m0->face_vertex_a[0], m0->face_vertex_b[0], m0->face_vertex_c[0]);
    assert(m0->face_vertex_a[0] == 0);
    assert(m0->face_vertex_b[0] == 1);
    assert(m0->face_vertex_c[0] == 2);
    assert(m0->face_colour[0] == 4550);

    // Scenery copies call this public function without an arena. This used to
    // dereference NULL when the first labelled Lumbridge scenery model loaded.
    Model heap_labels = {0};
    int32_t labels[] = {2, 0, 2};
    heap_labels.num_points = 3;
    heap_labels.vertex_label = labels;
    model_create_label_references(&heap_labels, NULL);
    assert(heap_labels.num_label_vertices == 3);
    assert(heap_labels.label_vertex_counts[0] == 1 && heap_labels.label_vertices[0][0] == 1);
    assert(heap_labels.label_vertex_counts[2] == 2 && heap_labels.label_vertices[2][1] == 2);
    for (int i = 0; i < heap_labels.num_label_vertices; i++) free(heap_labels.label_vertices[i]);
    free(heap_labels.label_vertices);
    free(heap_labels.label_vertex_counts);
    printf("  Heap-backed scenery label references verified\n");

    Model *textured=NULL;
    for(int id=0;id<1000;id++) {
        Model *candidate=model_load(id,&arena);
        if(candidate && candidate->num_t) { textured=candidate; break; }
    }
    assert(textured && textured->texture_a);
    Model *copy=model_copy(textured,&arena);
    assert(copy->num_t==textured->num_t && copy->texture_a!=textured->texture_a);
    Model *parts[2]={textured,copy};
    Model *combined=model_create_composite(parts,2,&arena);
    assert(combined && combined->num_t==2*textured->num_t);
    for(int t=0;t<combined->num_t;t++) {
        assert(combined->texture_a[t]>=0 && combined->texture_a[t]<combined->num_points);
        assert(combined->texture_b[t]>=0 && combined->texture_b[t]<combined->num_points);
        assert(combined->texture_c[t]>=0 && combined->texture_c[t]<combined->num_points);
    }
    for(int f=0;f<combined->num_faces;f++)
        if(combined->face_render_type[f]&2) assert((combined->face_render_type[f]>>2)<combined->num_t);
    puts("  Real cache texture axes, deep copies and composite remapping verified");

    // 2. Load Model 1
    Model *m1 = model_load(1, &arena);
    assert(m1 != NULL);
    printf("  Model 1 loaded: points=%d (expected 167), faces=%d (expected 302)\n",
           m1->num_points, m1->num_faces);
    assert(m1->num_points == 167);
    assert(m1->num_faces == 302);

    // 3. Test Lighting & Normal Calculation
    model_calculate_normals(m0, 64, 768, -50, -10, -50);
    assert(m0->face_colour_a != NULL);
    printf("  Model 0 lighting calculated successfully! Face 0 vertex A shaded = %d\n", m0->face_colour_a[0]);
    assert(m0->face_colour_a[0] > 0);

    // 4. Test 3D Projection & Rasterization into a framebuffer
    uint32_t *fb = arena_alloc(&arena, 500 * 500 * sizeof(uint32_t));
    pix2d_bind(fb, 500, 500);
    pix2d_cls();
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();

    model_render_obj(m0, 0, 512, 0, 0, 0, 0, 400);

    // Verify that pixels were drawn
    int drawn_count = 0;
    for (int i = 0; i < 500 * 500; i++) {
        if (fb[i] != 0) drawn_count++;
    }
    printf("  Model 0 rendered to framebuffer! Non-zero pixels = %d\n", drawn_count);
    assert(drawn_count > 100);

    arena_free(&arena);
    printf("[Test Model] ALL TESTS PASSED! Real 3D RS2 models successfully parsed and rendered.\n");
    return 0;
}
