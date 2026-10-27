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

int combat_wilderness_level225(int x,int z) {
    if(x>=2944 && x<3392 && z>=3520 && z<6400)return (z-3520)/8+1;
    if(x>=2944 && x<3392 && z>=9920 && z<12800)return (z-9920)/8+1;
    return 0;
}
int combat_world_location225(int x,int z) {
    if(x>=3328 && x<3392 && z>=3200 && z<3264){int a=x&63,b=z&63;
        if(((a>=4 && a<=29) || (a>=36 && a<=61)) && ((b>=44 && b<=58) || (b>=25 && b<=39) || (b>=6 && b<=20)))return 1;}
    return x>=3328 && x<=3393 && z>=3203 && z<=3325?2:0;
}
int combat_multiway_y225(int x,int z) {
    return combat_wilderness_level225(x,z)>0 || combat_world_location225(x,z)==1?258:296;
}
void combat_multiway_draw225(const Media *m,int flag,int x,int z,int vx,int vy) {
    if(!m || flag!=1 || !m->headicons[1])return;
    int l=pix2d_clip_left,t=pix2d_clip_top,r=pix2d_clip_right,b=pix2d_clip_bottom;
    pix2d_set_clip(l>vx?l:vx,t>vy?t:vy,r<vx+512?r:vx+512,b<vy+334?b:vy+334);
    pix32_draw(m->headicons[1],vx+472,vy+combat_multiway_y225(x,z));
    pix2d_set_clip(l,t,r,b);
}
void combat_location_draw225(const Media *m,PixFont *font,int flag,int x,int z,int vx,int vy) {
    combat_multiway_draw225(m,flag,x,z,vx,vy);
    if(!m)return;
    int level=combat_wilderness_level225(x,z),state=combat_world_location225(x,z);
    if(level<=0 && state!=1)return;
    int l=pix2d_clip_left,t=pix2d_clip_top,r=pix2d_clip_right,b=pix2d_clip_bottom;
    pix2d_set_clip(l>vx?l:vx,t>vy?t:vy,r<vx+512?r:vx+512,b<vy+334?b:vy+334);
    if(level>0) {
        if(m->headicons[0])pix32_draw(m->headicons[0],vx+472,vy+296);
        char text[24];snprintf(text,sizeof(text),"Level: %d",level);
        if(font)pixfont_draw_string_center(font,vx+484,vy+329,0xffff00,text);
    }
    if(state==1) {
        if(m->headicons[6])pix32_draw(m->headicons[6],vx+472,vy+296);
        if(font)pixfont_draw_string_center(font,vx+484,vy+329,0xffff00,"Arena");
    }
    pix2d_set_clip(l,t,r,b);
}
