#include "ui/context_menu.h"
#include "core/jstring.h"
#include "gfx/pix2d.h"
#include <string.h>
#include <stdio.h>

#define MENU_BORDER_COLOR 0x5D5447

bool context_menu_primary_opens225(const ContextMenu *menu,int mouse_buttons_option) {
    if(!menu || menu->visible || menu->size<=2 || menu->size>MAX_MENU_OPTIONS)return false;
    MenuAction a=menu->actions[menu->size-1];int code=a.target;if(code>=2000)code-=2000;
    bool add=a.type==MENU_SOCIAL?a.operation==1:a.type==MENU_LEGACY && code==406;
    return mouse_buttons_option==1 || add;
}

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
    menu->actions[menu->size] = (MenuAction){.type=MENU_LEGACY,.target=action_id};
    int code=action_id>=2000?action_id-2000:action_id;
    if(code==406 || code==436 || code==557 || code==556) {
        const char *tag=strstr(option,"@whi@");
        if(tag){const char *begin=tag+5;while(*begin && (unsigned char)*begin<=32)begin++;char name[80];snprintf(name,sizeof(name),"%s",begin);size_t n=strlen(name);while(n && (unsigned char)name[n-1]<=32)name[--n]=0;
            menu->actions[menu->size]=(MenuAction){.type=MENU_SOCIAL,.operation=code==406?1:code==557?2:code==436?4:5,.name37=jstring_to_base37(name),.low_priority=action_id>1000};}
    }
    if(code==34) {
        MenuAction *a=&menu->actions[menu->size];a->type=MENU_REPORT;a->operation=0;a->low_priority=action_id>1000;
        const char *tag=strstr(option,"@whi@");if(tag){const char *begin=tag+5;while(*begin && (unsigned char)*begin<=32)begin++;snprintf(a->report_name,sizeof(a->report_name),"%s",begin);size_t n=strlen(a->report_name);while(n && (unsigned char)a->report_name[n-1]<=32)a->report_name[--n]=0;a->operation=1;}
    }
    menu->action_ids[menu->size] = action_id;
    menu->size++;
}

void context_menu_add_action(ContextMenu *menu,const char *option,MenuAction action) {
    if(!menu || menu->size>=MAX_MENU_OPTIONS) return;
    int index=menu->size;
    context_menu_add_option(menu,option,0);
    if(menu->size>index) menu->actions[index]=action;
}
static bool low_priority(MenuAction a) {
    return a.low_priority || a.type==MENU_CANCEL || a.type==MENU_ITEM_EXAMINE || a.type==MENU_NPC_EXAMINE
        || a.type==MENU_LOC_EXAMINE || a.type==MENU_OBJ_EXAMINE || (a.type==MENU_LEGACY && a.target>1000);
}
void context_menu_prioritize(ContextMenu *menu) {
    if(!menu || menu->visible) return;
    // Original client stably moves action codes >1000 below ordinary actions.
    for(int pass=0;pass<menu->size;pass++) for(int i=0;i+1<menu->size;i++) {
        if(low_priority(menu->actions[i]) || !low_priority(menu->actions[i+1])) continue;
        MenuAction action=menu->actions[i];menu->actions[i]=menu->actions[i+1];menu->actions[i+1]=action;
        int id=menu->action_ids[i];menu->action_ids[i]=menu->action_ids[i+1];menu->action_ids[i+1]=id;
        char text[64];memcpy(text,menu->options[i],64);memcpy(menu->options[i],menu->options[i+1],64);memcpy(menu->options[i+1],text,64);
    }
}
static int option_at(const ContextMenu *menu, int x, int y) {
    if (x <= menu->x || x >= menu->x + menu->width) return -1;
    for (int i = 0; i < menu->size; i++) {
        int baseline = menu->y + 31 + (menu->size - 1 - i) * 15;
        if (y > baseline - 13 && y < baseline + 3) return i;
    }
    return -1;
}

bool context_menu_pick_action(ContextMenu *menu,int x,int y,MenuAction *action) {
    if(!menu || !menu->visible || !action) return false;
    int index=option_at(menu,x,y);
    menu->visible=false;
    if(index<0) return false;
    *action=menu->actions[index];return true;
}

void context_menu_open225(ContextMenu *menu,int click_x,int click_y,PixFont *font) {
    if(!menu || !font || menu->size<=0 || menu->size>MAX_MENU_OPTIONS)return;
    int width=pixfont_string_width(font,"Choose Option");for(int i=0;i<menu->size;i++){int w=pixfont_string_width(font,menu->options[i]);if(w>width)width=w;}width+=8;
    int ox,oy,w,h,area;
    if(click_x>8 && click_x<520 && click_y>11 && click_y<345){ox=8;oy=11;w=512;h=334;area=0;}
    else if(click_x>562 && click_x<752 && click_y>231 && click_y<492){ox=562;oy=231;w=190;h=261;area=1;}
    else if(click_x>22 && click_x<501 && click_y>375 && click_y<471){ox=22;oy=375;w=479;h=96;area=2;}
    else return;
    int x=click_x-ox-width/2,y=click_y-oy,clamp_height=menu->size*15+21;
    if(area==0){if(x+width>w)x=w-width;if(x<0)x=0;if(y+clamp_height>h)y=h-clamp_height;if(y<0)y=0;}
    else {if(x<0)x=0;else if(x+width>w)x=w-width;if(y<0)y=0;else if(y+clamp_height>h)y=h-clamp_height;}
    menu->x=x+ox;menu->y=y+oy;menu->width=width;menu->height=menu->size*15+22;menu->visible=true;
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

    if (mx + max_w > pix2d_width) mx = pix2d_width - max_w;
    if (mx < 0) mx = 0;
    if (my + total_h > pix2d_height) my = pix2d_height - total_h;
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
    // Original option area keeps the brown fill inside a black outline.
    pix2d_draw_rect(x + 1, y + 18, w - 2, h - 19, 0x000000);

    // "Choose Option" header
    pixfont_draw_string(font, x + 3, y + 14, MENU_BORDER_COLOR, "Choose Option");

    // Options (drawn top to bottom in reverse order of addition, matching RS2)
    for (int i = 0; i < menu->size; i++) {
        int opt_idx = menu->size - 1 - i;
        int opt_y = y + 31 + i * 15;

        uint32_t color = 0xFFFFFF; // white by default
        if (mouse_x > x && mouse_x < x + w && mouse_y > opt_y - 13 && mouse_y < opt_y + 3) {
            color = 0xFFFF00; // yellow when hovered
        }

        pixfont_draw_string_taggable(font, x + 3, opt_y, color, menu->options[opt_idx], true);
    }
}

int context_menu_handle_click(ContextMenu *menu, int mouse_x, int mouse_y) {
    if (!menu || !menu->visible) return -1;

    int index = option_at(menu, mouse_x, mouse_y);
    menu->visible = false;
    return index >= 0 ? menu->action_ids[index] : -1;
}
