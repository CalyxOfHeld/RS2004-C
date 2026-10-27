#include "ui/context_menu.h"
#include "gfx/pix2d.h"
#include <string.h>
#include <stdio.h>

#define MENU_BORDER_COLOR 0x5D5447

void context_menu_init(ContextMenu *menu) {
    if (!menu) return;
    memset(menu, 0, sizeof(ContextMenu));
}

void context_menu_clear(ContextMenu *menu) {
    if (!menu) return;
    menu->size = 0;
}

void context_menu_add_option(ContextMenu *menu, const char *option, int action_id) {
    if (!menu || !option || menu->size >= MAX_MENU_OPTIONS) return;
    strncpy(menu->options[menu->size], option, sizeof(menu->options[0]) - 1);
    menu->options[menu->size][sizeof(menu->options[0]) - 1] = '\0';
    menu->action_ids[menu->size] = action_id;
    menu->size++;
}

void context_menu_open(ContextMenu *menu, int click_x, int click_y, PixFont *font) {
    if (!menu || menu->size == 0) return;

    int max_w = pixfont_string_width(font, "Choose Option");
    for (int i = 0; i < menu->size; i++) {
        int w = pixfont_string_width(font, menu->options[i]);
        if (w > max_w) max_w = w;
    }
    max_w += 8;
    int total_h = menu->size * 15 + 22;

    int mx = click_x - max_w / 2;
    int my = click_y;

    if (mx + max_w > 765) mx = 765 - max_w;
    if (mx < 0) mx = 0;
    if (my + total_h > 503) my = 503 - total_h;
    if (my < 0) my = 0;

    menu->x = mx;
    menu->y = my;
    menu->width = max_w;
    menu->height = total_h;
    menu->visible = true;
}

void context_menu_draw(ContextMenu *menu, int mouse_x, int mouse_y, PixFont *font) {
    if (!menu || !menu->visible || menu->size == 0 || !font) return;

    int x = menu->x;
    int y = menu->y;
    int w = menu->width;
    int h = menu->height;

    // Outer border
    pix2d_fill_rect(x, y, w, h, MENU_BORDER_COLOR);
    // Header black background
    pix2d_fill_rect(x + 1, y + 1, w - 2, 16, 0x000000);
    // Options black background
    pix2d_fill_rect(x + 1, y + 18, w - 2, h - 19, 0x000000);

    // "Choose Option" header
    pixfont_draw_string(font, x + 3, y + 14, MENU_BORDER_COLOR, "Choose Option");

    // Options (drawn top to bottom in reverse order of addition, matching RS2)
    for (int i = 0; i < menu->size; i++) {
        int opt_idx = menu->size - 1 - i;
        int opt_y = y + 31 + i * 15;

        uint32_t color = 0xFFFFFF; // white by default
        if (mouse_x >= x && mouse_x < x + w && mouse_y > opt_y - 13 && mouse_y <= opt_y + 2) {
            color = 0xFFFF00; // yellow when hovered
        }

        pixfont_draw_string_taggable(font, x + 3, opt_y, color, menu->options[opt_idx], true);
    }
}

int context_menu_handle_click(ContextMenu *menu, int mouse_x, int mouse_y) {
    if (!menu || !menu->visible) return -1;

    int x = menu->x;
    int y = menu->y;
    int w = menu->width;

    menu->visible = false;

    if (mouse_x >= x && mouse_x < x + w && mouse_y >= y + 18 && mouse_y < y + menu->height) {
        int opt_slot = (mouse_y - (y + 18)) / 15;
        if (opt_slot >= 0 && opt_slot < menu->size) {
            int opt_idx = menu->size - 1 - opt_slot;
            return menu->action_ids[opt_idx];
        }
    }

    return -1; // Clicked outside or cancelled
}
