#ifndef RS2_INPUT_TRACKING_SDL_H
#define RS2_INPUT_TRACKING_SDL_H
#include "client/client.h"
#include <SDL2/SDL_events.h>
#include <string.h>
/* Plain/shift/caps US ASCII translation. Core tracking accepts all normalized
 * GameShell values; other keyboard layouts/IME need separate qualification. */
static int tracking_sdl_character(SDL_Keycode key,SDL_Keymod mods){
    int code=65535;bool shift=(mods&KMOD_SHIFT)!=0;
    if(key>=32 && key<127){code=(int)key;
        if(key>='a' && key<='z'){if(shift != ((mods&KMOD_CAPS)!=0))code-=32;}
        else if(shift){const char *plain="`1234567890-=[]\\;',./",*upper="~!@#$%^&*()_+{}|:\"<>?";const char *at=strchr(plain,(char)key);if(at)code=upper[at-plain];}
        if((mods&KMOD_CTRL) && ((code>='A' && code<='Z') || (code>='a' && code<='z') || (code>='[' && code<='_') || code=='@'))code&=31;
    }else if(key>=160 && key<=65535)code=(int)key;
    return code<30?0:code;
}
static bool tracking_sdl_event(const SDL_Event *event,ClientEvent *out){
    memset(out,0,sizeof(*out));
    if(event->type==SDL_WINDOWEVENT){switch(event->window.event){
        case SDL_WINDOWEVENT_FOCUS_GAINED:out->type=CLIENT_EVENT_FOCUS_GAIN;return true;
        case SDL_WINDOWEVENT_FOCUS_LOST:out->type=CLIENT_EVENT_FOCUS_LOSS;return true;
        case SDL_WINDOWEVENT_ENTER:out->type=CLIENT_EVENT_MOUSE_ENTER;return true;
        case SDL_WINDOWEVENT_LEAVE:out->type=CLIENT_EVENT_MOUSE_EXIT;return true;
        default:return false;}}
    if(event->type!=SDL_KEYDOWN && event->type!=SDL_KEYUP)return false;
    bool press=event->type==SDL_KEYDOWN;SDL_Keycode key=event->key.keysym.sym;int code=tracking_sdl_character(key,(SDL_Keymod)event->key.keysym.mod);
    switch(key){
        case SDLK_LEFT:code=1;break;case SDLK_RIGHT:code=2;break;case SDLK_UP:code=3;break;case SDLK_DOWN:code=4;break;
        case SDLK_LCTRL:case SDLK_RCTRL:code=5;break;
        case SDLK_BACKSPACE:case SDLK_DELETE:code=8;break;case SDLK_TAB:code=9;break;
        case SDLK_RETURN:case SDLK_KP_ENTER:code=10;break;case SDLK_ESCAPE:code=0;break;
        case SDLK_HOME:code=press?1000:65535;break;case SDLK_END:code=press?1001:65535;break;
        case SDLK_PAGEUP:code=press?1002:65535;break;case SDLK_PAGEDOWN:code=press?1003:65535;break;
        default:if(key>=SDLK_F1 && key<=SDLK_F12)code=press?1008+(int)(key-SDLK_F1):65535;break;
    }
    out->type=press?CLIENT_EVENT_TRACK_KEY_DOWN:CLIENT_EVENT_TRACK_KEY_UP;out->key=code;return true;
}
#endif
