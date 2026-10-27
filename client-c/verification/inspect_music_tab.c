#include <stdio.h>
#include <stdlib.h>
#include "ui/component.h"
#include "ui/varp.h"
#include "core/arena.h"

int main(void) {
    Arena arena = arena_create(16 * 1024 * 1024);
    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena);
    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);

    ComponentConfig cfg;
    PixFont *fonts[4] = {0};
    component_unpack(&cfg, media_jag, fonts, interface_jag, &arena);

    Component *root = component_get(&cfg, 962);
    printf("Component 962: type=%d width=%d height=%d child_count=%d\n", root->type, root->width, root->height, root->child_count);
    for (int i = 0; i < root->child_count; i++) {
        Component *child = component_get(&cfg, root->child_id[i]);
        printf("  Child #%d: id=%d x=%d y=%d type=%d text='%s'\n",
            i, root->child_id[i], root->child_x[i], root->child_y[i], child ? child->type : -1, (child && child->text) ? child->text : "");
        if (child && child->type == COMPONENT_TYPE_CONTAINER) {
            printf("    Container child_count=%d scroll=%d height=%d\n", child->child_count, child->scroll, child->height);
            for (int k = 0; k < child->child_count && k < 10; k++) {
                Component *grand = component_get(&cfg, child->child_id[k]);
                printf("      Grand #%d: id=%d x=%d y=%d text='%s'\n",
                    k, child->child_id[k], child->child_x[k], child->child_y[k], (grand && grand->text) ? grand->text : "");
            }
        }
    }
    return 0;
}
