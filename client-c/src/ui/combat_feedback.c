#include "ui/combat_feedback.h"
#include "gfx/pix2d.h"
#include <stdio.h>
void combat_feedback_draw(const PathingEntity *e,int cycle,const Media *m,PixFont *font,int bx,int by,int hx,int hy) {
    int l=pix2d_clip_left,t=pix2d_clip_top,r=pix2d_clip_right,b=pix2d_clip_bottom;
    pix2d_set_clip(l>8?l:8,t>11?t:11,r<520?r:520,b<345?b:345);
    if(e->combat_cycle>cycle+100 && bx>=0 && e->total_health>0) {
        int green=e->health*30/e->total_health;if(green>30)green=30;if(green<0)green=0;
        pix2d_fill_rect(bx-15,by-3,green,5,0x00ff00);
        pix2d_fill_rect(bx-15+green,by-3,30-green,5,0xff0000);
    }
    if(e->combat_cycle>cycle+330 && hx>=0 && e->damage_type>=0 && e->damage_type<20 && m->hitmarks[e->damage_type]) {
        pix32_draw(m->hitmarks[e->damage_type],hx-12,hy-12);
        char damage[16];snprintf(damage,sizeof(damage),"%d",e->damage);
        pixfont_draw_string_center(font,hx,hy+4,0,damage);
        pixfont_draw_string_center(font,hx-1,hy+3,0xffffff,damage);
    }
    pix2d_set_clip(l,t,r,b);
}

int combat_multiway_y225(int x,int z) {
    bool wilderness=x>=2944 && x<3392 && ((z>=3520 && z<6400) || (z>=9920 && z<12800));
    bool arena=false;
    if(x>=3328 && x<3392 && z>=3200 && z<3264){int a=x&63,b=z&63;arena=((a>=4 && a<=29) || (a>=36 && a<=61)) && ((b>=44 && b<=58) || (b>=25 && b<=39) || (b>=6 && b<=20));}
    return wilderness || arena?258:296;
}
void combat_multiway_draw225(const Media *m,int flag,int x,int z,int vx,int vy) {
    if(!m || flag!=1 || !m->headicons[1])return;
    int l=pix2d_clip_left,t=pix2d_clip_top,r=pix2d_clip_right,b=pix2d_clip_bottom;
    pix2d_set_clip(l>vx?l:vx,t>vy?t:vy,r<vx+512?r:vx+512,b<vy+334?b:vy+334);
    pix32_draw(m->headicons[1],vx+472,vy+combat_multiway_y225(x,z));
    pix2d_set_clip(l,t,r,b);
}
