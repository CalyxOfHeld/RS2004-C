/* Native transport only. The original client decides song/jingle/Off/On.
 * One worker owns the Windows synth and MIDI timeline. Volume applies output dB;
 * it never opens/closes/resets the device or changes the event cursor/clock. */
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include "midi_sequence225.h"
#include "midi_output225.h"
typedef struct {
    SDL_mutex *mutex; SDL_Thread *thread;
    bool quit,playing; int command,volume; unsigned generation;
    uint8_t *pending; size_t length; bool ambient;
} MidiMailbox225;
static MidiMailbox225 md225;
static MidiOutput225 *md_output225;
static uint8_t *md_base225;
static MidiSequence225 md_sequence225;
static size_t md_cursor225;
static Uint64 md_start225;
static bool md_ambient225,md_active225;
static int md_volume225;
static void md_lock225(void){SDL_LockMutex(md225.mutex);}
static void md_unlock225(void){SDL_UnlockMutex(md225.mutex);}
static bool md_current225(unsigned generation){md_lock225();bool current=!md225.quit && generation==md225.generation;md_unlock225();return current;}
static void md_publish225(bool playing,unsigned generation){md_lock225();if(generation==md225.generation)md225.playing=playing;md_unlock225();}
static bool md_short225(DWORD message){return mo_send225(md_output225,message,NULL,0);}
static bool md_gain225(void){return mo_gain225(md_output225,md_volume225);}
static bool md_sysex225(const MidiEvent225 *e){
    size_t extra=e->kind==2?1:0,size=e->length+extra;if(!size)return true;
    uint8_t *bytes=malloc(size);if(!bytes)return false;
    if(extra)bytes[0]=0xf0;
    memcpy(bytes+extra,e->data,e->length);bool ok=mo_send225(md_output225,0,bytes,size);free(bytes);return ok;
}
static void md_release_song225(void){
    if(md_output225)mo_reset225(md_output225);
    ms_free225(&md_sequence225);free(md_base225);md_base225=NULL;md_active225=false;md_cursor225=0;
}
static bool md_begin225(void){
    if(!mo_start225(md_output225))return false;
    /* Reset only at a song boundary, never for a volume adjustment. */
    for(unsigned ch=0;ch<16;ch++){
        if(!md_short225((0xb0u+ch)|(121u<<8)) || !md_short225((0xb0u+ch)|(7u<<8)|(100u<<16)) || !md_short225((0xc0u+ch)))return false;
    }
    if(!md_gain225())return false;
    md_cursor225=0;md_start225=SDL_GetPerformanceCounter();md_active225=true;return true;
}
static bool md_event225(const MidiEvent225 *e){
    if(e->kind>=2 && e->kind<=3)return md_sysex225(e);
    if(e->kind)return true;
    return md_short225(e->message);
}
static void md_step225(void){
    md_lock225();int command=md225.command,volume=md225.volume;unsigned generation=md225.generation;
    uint8_t *bytes=md225.pending;size_t length=md225.length;bool ambient=md225.ambient;
    md225.pending=NULL;md225.length=0;md225.command=0;md_unlock225();
    bool changed=volume!=md_volume225;md_volume225=volume;
    if(command){
        md_release_song225();
        if(command==1 && md_current225(generation)){
            md_base225=bytes;bytes=NULL;md_ambient225=ambient;
            if(!md_output225 && !mo_open225(&md_output225,md_volume225))md_output225=NULL;
            if(md_output225 && md_current225(generation) && ms_parse225(md_base225,length,&md_sequence225) && md_begin225())md_publish225(ambient,generation);
            else{fprintf(stderr,"[MIDI] Song rejected or output unavailable\n");md_release_song225();md_publish225(false,generation);}
        }else md_publish225(false,generation);
    }else if(changed && md_active225 && !md_gain225()){md_release_song225();md_publish225(false,generation);}
    free(bytes);
    if(!md_active225 || !md_current225(generation))return;
    Uint64 elapsed=(SDL_GetPerformanceCounter()-md_start225)*1000000u/SDL_GetPerformanceFrequency();
    while(md_cursor225<md_sequence225.count && md_sequence225.events[md_cursor225].us<=elapsed){
        if(!md_current225(generation))return;
        if(!md_event225(&md_sequence225.events[md_cursor225++])){md_release_song225();md_publish225(false,generation);return;}
    }
    if(md_cursor225==md_sequence225.count){
        if(md_ambient225 && md_sequence225.duration && md_current225(generation)){
            mo_reset225(md_output225);
            md_publish225(md_begin225(),generation);
        }else {mo_reset225(md_output225);md_active225=false;md_publish225(false,generation);}
    }
}
#ifndef MIDI_DEVICE_SYNC_CONTROL
static int md_worker225(void *unused){
    (void)unused;bool timer=timeBeginPeriod(1)==TIMERR_NOERROR;
    for(;;){md_lock225();bool quit=md225.quit;md_unlock225();if(quit)break;md_step225();SDL_Delay(1);}
    md_release_song225();if(md_output225){mo_close225(md_output225);md_output225=NULL;}
    if(timer)timeEndPeriod(1);
    return 0;
}
#endif
static bool md_ensure225(int volume){
    if(md225.mutex)return true;
    md225.mutex=SDL_CreateMutex();if(!md225.mutex)return false;md225.volume=md_volume225=volume;
#ifndef MIDI_DEVICE_SYNC_CONTROL
    md225.thread=SDL_CreateThread(md_worker225,"midi225",NULL);
    if(!md225.thread){SDL_DestroyMutex(md225.mutex);memset(&md225,0,sizeof(md225));return false;}
#endif
    return true;
}
static void md_kick225(void){
#ifdef MIDI_DEVICE_SYNC_CONTROL
    if(md225.mutex)md_step225();
#endif
}
static bool md_submit225(const uint8_t *bytes,size_t length,bool ambient,int volume){
    uint8_t *copy=malloc(length);if(!copy)return false;memcpy(copy,bytes,length);
    if(!md_ensure225(volume)){free(copy);return false;}
    md_lock225();uint8_t *old=md225.pending;md225.pending=copy;md225.length=length;md225.ambient=ambient;md225.command=1;md225.volume=volume;md225.generation++;md225.playing=ambient;md_unlock225();free(old);
    md_kick225();return true;
}
static void md_stop225(void){
    if(!md225.mutex)return;
    md_lock225();uint8_t *old=md225.pending;md225.pending=NULL;md225.length=0;md225.command=2;md225.generation++;md225.playing=false;md_unlock225();free(old);md_kick225();
}
static void md_set_volume225(int volume){if(!md225.mutex)return;md_lock225();md225.volume=volume;md_unlock225();md_kick225();}
static bool md_playing225(void){if(!md225.mutex)return false;md_kick225();md_lock225();bool playing=md225.playing;md_unlock225();return playing;}
static void md_close225(void){
    if(!md225.mutex)return;
    md_lock225();md225.quit=true;md_unlock225();
#ifndef MIDI_DEVICE_SYNC_CONTROL
    SDL_WaitThread(md225.thread,NULL);
#else
    md_release_song225();if(md_output225){mo_close225(md_output225);md_output225=NULL;}
#endif
    free(md225.pending);SDL_DestroyMutex(md225.mutex);memset(&md225,0,sizeof(md225));
}
#else
static bool md_submit225(const uint8_t *b,size_t n,bool a,int v){(void)b;(void)n;(void)a;(void)v;return false;}
static void md_stop225(void){}
static void md_set_volume225(int v){(void)v;}
static bool md_playing225(void){return false;}
static void md_close225(void){}
#endif
