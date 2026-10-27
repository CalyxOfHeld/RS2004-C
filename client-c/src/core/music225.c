#include "core/music225.h"
#include <string.h>

void music_init225(Music225 *m){if(m){memset(m,0,sizeof(*m));m->active=true;}}
static void request225(Music225 *m){
    if(!m->has_current)return;
    memcpy(m->requested,m->current,sizeof(m->requested));
    m->requested_crc=m->crc;m->requested_size=m->size;m->pending=true;
}
void music_setting225(Music225 *m,int value){
    if(!m)return;
    bool previous=m->active;
    if(value>=0 && value<=3){m->volume=-400*value;if(previous)m->command=1;m->active=true;}
    if(value==4)m->active=false;
    if(m->active!=previous){if(m->active)request225(m);else {m->command=2;m->fade=0;}m->delay=0;}
}
bool music_song225(Music225 *m,const char *name,uint32_t crc,uint32_t size){
    if(!m || !name || strlen(name)>=sizeof(m->current))return false;
    bool changed=!m->has_current || strcmp(m->current,name)!=0;
    strcpy(m->current,name);m->has_current=true;m->crc=crc;m->size=size;m->delay=0;
    if(changed && m->active && !m->low_memory)request225(m);
    return true;
}
bool music_jingle225(Music225 *m,int delay){
    if(!m || !m->active || m->low_memory)return false;
    m->fade=0;m->delay=delay;return true;
}
void music_tick225(Music225 *m){
    if(!m || m->delay<=0)return;
    m->delay-=20;if(m->delay<0)m->delay=0;
    if(!m->delay && m->active && !m->low_memory)request225(m);
}
void music_logout225(Music225 *m){if(m){m->has_current=false;m->current[0]=0;m->delay=0;m->command=2;m->fade=0;}}
