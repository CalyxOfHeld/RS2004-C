#ifndef RS2_MEDIA_H
#define RS2_MEDIA_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix8.h"
#include "gfx/pix32.h"

typedef struct {
    Pix8 *invback;
    Pix8 *chatback;
    Pix8 *mapback;
    Pix8 *backbase1;
    Pix8 *backbase2;
    Pix8 *backhmid1;

    Pix32 *backleft1;
    Pix32 *backleft2;
    Pix32 *backright1;
    Pix32 *backright2;
    Pix32 *backtop1;
    Pix32 *backtop2;
    Pix32 *backvmid1;
    Pix32 *backvmid2;
    Pix32 *backvmid3;
    Pix32 *backhmid2;

    Pix8 *sideicons[14];
    Pix32 *compass;
    Pix32 *crosses[8];
    Pix32 *mapdots[4];
    Pix32 *mapflag;
    Pix8 *mapscene[50];
    Pix32 *mapfunction[50];
    Pix32 *headicons[20];

    Pix8 *redstone1;
    Pix8 *redstone2;
    Pix8 *redstone3;
    Pix8 *redstone1h;
    Pix8 *redstone2h;
    Pix8 *redstone1v;
    Pix8 *redstone2v;
    Pix8 *redstone3v;
    Pix8 *redstone1hv;
    Pix8 *redstone2hv;

    Pix8 *scrollbar0;
    Pix8 *scrollbar1;
} Media;

bool media_load(Media *m, Jagfile *media_jag, Arena *arena);
void media_draw_shell(Media *m);
void media_draw_tabs(Media *m, int selected_tab);
void media_draw_tabs_flashing(Media *m, int selected_tab, const int *tab_interfaces, int flashing_tab, int loop_cycle);

#endif // RS2_MEDIA_H
