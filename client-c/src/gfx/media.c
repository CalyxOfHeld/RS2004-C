#include "gfx/media.h"
#include <string.h>
#include <stdio.h>

bool media_load(Media *m, Jagfile *media_jag, Arena *arena) {
    if (!m || !media_jag) return false;
    memset(m, 0, sizeof(Media));

    m->invback = pix8_load(media_jag, "invback", 0, arena);
    m->chatback = pix8_load(media_jag, "chatback", 0, arena);
    m->mapback = pix8_load(media_jag, "mapback", 0, arena);
    m->backbase1 = pix8_load(media_jag, "backbase1", 0, arena);
    m->backbase2 = pix8_load(media_jag, "backbase2", 0, arena);
    m->backhmid1 = pix8_load(media_jag, "backhmid1", 0, arena);

    m->backleft1 = pix32_load(media_jag, "backleft1", 0, arena);
    m->backleft2 = pix32_load(media_jag, "backleft2", 0, arena);
    m->backright1 = pix32_load(media_jag, "backright1", 0, arena);
    m->backright2 = pix32_load(media_jag, "backright2", 0, arena);
    m->backtop1 = pix32_load(media_jag, "backtop1", 0, arena);
    m->backtop2 = pix32_load(media_jag, "backtop2", 0, arena);
    m->backvmid1 = pix32_load(media_jag, "backvmid1", 0, arena);
    m->backvmid2 = pix32_load(media_jag, "backvmid2", 0, arena);
    m->backvmid3 = pix32_load(media_jag, "backvmid3", 0, arena);
    m->backhmid2 = pix32_load(media_jag, "backhmid2", 0, arena);

    for (int i = 0; i < 13; i++) {
        m->sideicons[i] = pix8_load(media_jag, "sideicons", i, arena);
    }

    m->compass = pix32_load(media_jag, "compass", 0, arena);
    for (int i = 0; i < 8; i++) {
        m->crosses[i] = pix32_load(media_jag, "cross", i, arena);
    }
    for (int i = 0; i < 4; i++) {
        m->mapdots[i] = pix32_load(media_jag, "mapdots", i, arena);
    }

    for(int i=0;i<50;i++) {
        m->mapscene[i]=pix8_load(media_jag,"mapscene",i,arena);
        if(!m->mapscene[i]) break;
    }
    for(int i=0;i<50;i++) {
        m->mapfunction[i]=pix32_load(media_jag,"mapfunction",i,arena);
        if(!m->mapfunction[i]) break;
    }

    m->redstone1 = pix8_load(media_jag, "redstone1", 0, arena);
    m->redstone2 = pix8_load(media_jag, "redstone2", 0, arena);
    m->redstone3 = pix8_load(media_jag, "redstone3", 0, arena);

    m->redstone1h = pix8_load(media_jag, "redstone1", 0, arena);
    if (m->redstone1h) pix8_flip_horizontally(m->redstone1h, arena);
    m->redstone2h = pix8_load(media_jag, "redstone2", 0, arena);
    if (m->redstone2h) pix8_flip_horizontally(m->redstone2h, arena);

    m->redstone1v = pix8_load(media_jag, "redstone1", 0, arena);
    if (m->redstone1v) pix8_flip_vertically(m->redstone1v, arena);
    m->redstone2v = pix8_load(media_jag, "redstone2", 0, arena);
    if (m->redstone2v) pix8_flip_vertically(m->redstone2v, arena);
    m->redstone3v = pix8_load(media_jag, "redstone3", 0, arena);
    if (m->redstone3v) pix8_flip_vertically(m->redstone3v, arena);

    m->redstone1hv = pix8_load(media_jag, "redstone1", 0, arena);
    if (m->redstone1hv) {
        pix8_flip_horizontally(m->redstone1hv, arena);
        pix8_flip_vertically(m->redstone1hv, arena);
    }
    m->redstone2hv = pix8_load(media_jag, "redstone2", 0, arena);
    if (m->redstone2hv) {
        pix8_flip_horizontally(m->redstone2hv, arena);
        pix8_flip_vertically(m->redstone2hv, arena);
    }

    m->scrollbar0 = pix8_load(media_jag, "scrollbar", 0, arena);
    m->scrollbar1 = pix8_load(media_jag, "scrollbar", 1, arena);

    return true;
}

void media_draw_shell(Media *m) {
    if (!m) return;

    if (m->backtop1) pix32_draw(m->backtop1, 0, 0);
    if (m->backtop2) pix32_draw(m->backtop2, 561, 0);
    if (m->backleft1) pix32_draw(m->backleft1, 0, 11);
    if (m->backleft2) pix32_draw(m->backleft2, 0, 375);
    if (m->backvmid1) pix32_draw(m->backvmid1, 520, 11);
    if (m->backvmid2) pix32_draw(m->backvmid2, 520, 231);
    if (m->backvmid3) pix32_draw(m->backvmid3, 501, 375);
    if (m->backright1) pix32_draw(m->backright1, 729, 5);
    if (m->backright2) pix32_draw(m->backright2, 752, 231);
    if (m->backhmid2) pix32_draw(m->backhmid2, 0, 345);
    if (m->backbase1) pix8_draw(m->backbase1, 0, 471);
    if (m->backbase2) pix8_draw(m->backbase2, 501, 492);
    if (m->mapback) pix8_draw(m->mapback, 561, 5);
    if (m->chatback) pix8_draw(m->chatback, 22, 375);
    if (m->invback) pix8_draw(m->invback, 562, 231);
}

void media_draw_tabs_flashing(Media *m, int selected_tab, const int *tab_interfaces, int flashing_tab, int loop_cycle) {
    if (!m) return;

    // Draw top stone bar
    if (m->backhmid1) pix8_draw(m->backhmid1, 520, 165);

    // Helper macro to check if tab is active/unlocked
    #define TAB_ACTIVE(i) (tab_interfaces == NULL || (tab_interfaces[i] > 0 && tab_interfaces[i] != 65535))

    // Draw selected redstone under top tabs (only if tab is unlocked/active)
    if (TAB_ACTIVE(selected_tab)) {
        if (selected_tab == 0 && m->redstone1) pix8_draw(m->redstone1, 520 + 29, 165 + 30);
        if (selected_tab == 1 && m->redstone2) pix8_draw(m->redstone2, 520 + 59, 165 + 29);
        if (selected_tab == 2 && m->redstone2) pix8_draw(m->redstone2, 520 + 87, 165 + 29);
        if (selected_tab == 3 && m->redstone3) pix8_draw(m->redstone3, 520 + 115, 165 + 29);
        if (selected_tab == 4 && m->redstone2h) pix8_draw(m->redstone2h, 520 + 156, 165 + 29);
        if (selected_tab == 5 && m->redstone2h) pix8_draw(m->redstone2h, 520 + 184, 165 + 29);
        if (selected_tab == 6 && m->redstone1h) pix8_draw(m->redstone1h, 520 + 212, 165 + 30);
    }

    // Helper macro to check if sideicon should draw (either unlocked or flashing blink cycle)
    #define SHOULD_DRAW_ICON(i) (TAB_ACTIVE(i) ? (flashing_tab != (i) || (loop_cycle % 20 < 10)) : (flashing_tab == (i) && (loop_cycle % 20 < 10)))

    // Draw top row sideicons
    if (m->sideicons[0] && SHOULD_DRAW_ICON(0)) pix8_draw(m->sideicons[0], 520 + 35, 165 + 34);
    if (m->sideicons[1] && SHOULD_DRAW_ICON(1)) pix8_draw(m->sideicons[1], 520 + 59, 165 + 32);
    if (m->sideicons[2] && SHOULD_DRAW_ICON(2)) pix8_draw(m->sideicons[2], 520 + 86, 165 + 32);
    if (m->sideicons[3] && SHOULD_DRAW_ICON(3)) pix8_draw(m->sideicons[3], 520 + 121, 165 + 33);
    if (m->sideicons[4] && SHOULD_DRAW_ICON(4)) pix8_draw(m->sideicons[4], 520 + 157, 165 + 34);
    if (m->sideicons[5] && SHOULD_DRAW_ICON(5)) pix8_draw(m->sideicons[5], 520 + 185, 165 + 32);
    if (m->sideicons[6] && SHOULD_DRAW_ICON(6)) pix8_draw(m->sideicons[6], 520 + 212, 165 + 34);

    // Draw selected redstone under bottom tabs
    if (TAB_ACTIVE(selected_tab)) {
        if (selected_tab == 7 && m->redstone1v) pix8_draw(m->redstone1v, 501 + 49, 492 + 0);
        if (selected_tab == 8 && m->redstone2v) pix8_draw(m->redstone2v, 501 + 81, 492 + 0);
        if (selected_tab == 9 && m->redstone2v) pix8_draw(m->redstone2v, 501 + 108, 492 + 0);
        if (selected_tab == 10 && m->redstone3v) pix8_draw(m->redstone3v, 501 + 136, 492 + 1);
        if (selected_tab == 11 && m->redstone2hv) pix8_draw(m->redstone2hv, 501 + 178, 492 + 0);
        if (selected_tab == 12 && m->redstone2hv) pix8_draw(m->redstone2hv, 501 + 205, 492 + 0);
        if (selected_tab == 13 && m->redstone1hv) pix8_draw(m->redstone1hv, 501 + 233, 492 + 0);
    }

    // Draw bottom row sideicons (sideicons[7..12] map to tabs 8..13)
    if (m->sideicons[7] && SHOULD_DRAW_ICON(8)) pix8_draw(m->sideicons[7], 501 + 80, 492 + 2);
    if (m->sideicons[8] && SHOULD_DRAW_ICON(9)) pix8_draw(m->sideicons[8], 501 + 107, 492 + 3);
    if (m->sideicons[9] && SHOULD_DRAW_ICON(10)) pix8_draw(m->sideicons[9], 501 + 142, 492 + 4);
    if (m->sideicons[10] && SHOULD_DRAW_ICON(11)) pix8_draw(m->sideicons[10], 501 + 179, 492 + 2);
    if (m->sideicons[11] && SHOULD_DRAW_ICON(12)) pix8_draw(m->sideicons[11], 501 + 206, 492 + 2);
    if (m->sideicons[12] && SHOULD_DRAW_ICON(13)) pix8_draw(m->sideicons[12], 501 + 230, 492 + 2);

    #undef TAB_ACTIVE
    #undef SHOULD_DRAW_ICON
}

void media_draw_tabs(Media *m, int selected_tab) {
    media_draw_tabs_flashing(m, selected_tab, NULL, -1, 0);
}
