#ifndef INPUT_TRACKING_REFERENCE_CONTROL_H
#define INPUT_TRACKING_REFERENCE_CONTROL_H
#include "net/protocol.h"
#include "client/input_tracking_sdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int tr_record;
static int tr_sdl_checks;
static void tr_check(bool ok,const char *label){if(!ok){fprintf(stderr,"tracking record%d: %s\n",tr_record,label);exit(3);}}
static uint32_t tr_u32(FILE *f){uint8_t b[4];tr_check(fread(b,1,4,f)==4,"fixture integer");return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int64_t tr_i64(FILE *f){uint64_t bits=(uint64_t)tr_u32(f)<<32;bits|=tr_u32(f);int64_t result;memcpy(&result,&bits,8);return result;}
static void tr_equal(FILE *f,int value,const char *label){int expected=(int32_t)tr_u32(f);if(value!=expected){fprintf(stderr,"tracking record%d %s actual%d expected%d\n",tr_record,label,value,expected);exit(3);}}
static bool tr_bool(FILE *f){int value=fgetc(f);tr_check(value==0 || value==1,"fixture boolean");return value!=0;}
static void tr_buffer(FILE *f,bool present,const uint8_t *bytes,size_t length){int n=(int32_t)tr_u32(f);tr_check((n>=0)==present,"buffer presence");if(n>=0){uint8_t expected[500];tr_check(n<500 && length==(size_t)n && fread(expected,1,(size_t)n,f)==(size_t)n && !memcmp(bytes,expected,(size_t)n),"complete buffer prefix");}}
static void tr_cipher(FILE *f,const Isaac *cipher){tr_equal(f,cipher->count,"cipher count");tr_equal(f,(int32_t)cipher->a,"cipher a");tr_equal(f,(int32_t)cipher->b,"cipher b");tr_equal(f,(int32_t)cipher->c,"cipher c");for(int i=0;i<256;i++)tr_equal(f,(int32_t)cipher->rsl[i],"cipher rsl");for(int i=0;i<256;i++)tr_equal(f,(int32_t)cipher->mem[i],"cipher mem");}
typedef struct {void (*action)(int,int,int,int,int64_t);void (*wire)(GameSession*,int);int (*clock_calls)(void);} TrackingHooks;
static void tr_sdl_case(int op,SDL_Keycode key,SDL_Keymod mods,uint8_t expected){
    SDL_Event sdl;memset(&sdl,0,sizeof(sdl));sdl.type=op==8?SDL_KEYDOWN:SDL_KEYUP;sdl.key.keysym.sym=key;sdl.key.keysym.mod=(Uint16)mods;ClientEvent event;tr_check(tracking_sdl_event(&sdl,&event),"SDL key translation");tr_check(event.type==(op==8?CLIENT_EVENT_TRACK_KEY_DOWN:CLIENT_EVENT_TRACK_KEY_UP),"SDL raw key phase");InputTracking t={0};input_tracking_enable(&t,100000);tr_check(input_tracking_event(&t,op,0,0,event.key,100010) && t.out_length==3 && t.out[0]==op && t.out[2]==expected,"SDL encoded key vs original GameShell");tr_sdl_checks++;
}
static void tr_sdl_probe(int op,int vk,int ch,int awt_mods,uint8_t expected){
    SDL_Keycode key=SDLK_UNKNOWN;bool known=true;SDL_Keymod mods=(awt_mods&64)?KMOD_SHIFT:KMOD_NONE;
    switch(vk){
        case 37:key=SDLK_LEFT;break;case 39:key=SDLK_RIGHT;break;case 38:key=SDLK_UP;break;case 40:key=SDLK_DOWN;break;case 17:key=SDLK_LCTRL;break;
        case 8:key=SDLK_BACKSPACE;break;case 127:key=SDLK_DELETE;break;case 9:key=SDLK_TAB;break;case 10:key=SDLK_RETURN;break;
        case 27:key=SDLK_ESCAPE;known=ch<30;break;case 16:key=SDLK_LSHIFT;known=ch==65535;break;case 18:key=SDLK_LALT;known=ch==65535;break;
        case 36:key=SDLK_HOME;known=ch==65535;break;case 35:key=SDLK_END;known=ch==65535;break;case 33:key=SDLK_PAGEUP;known=ch==65535;break;case 34:key=SDLK_PAGEDOWN;known=ch==65535;break;
        default:
            if(vk>=112 && vk<=123){key=SDLK_F1+vk-112;known=ch==65535;}
            else if(vk>=65 && vk<=90 && (ch==vk || ch==vk+32)){key=vk+32;if((ch==vk && !(mods&KMOD_SHIFT)) || (ch==vk+32 && (mods&KMOD_SHIFT)))mods=(SDL_Keymod)(mods|KMOD_CAPS);}
            else if(vk>=48 && vk<=57)key=vk;
            else {switch(vk){case 32:key=SDLK_SPACE;break;case 44:key=SDLK_COMMA;break;case 45:key=SDLK_MINUS;break;case 46:key=SDLK_PERIOD;break;case 47:key=SDLK_SLASH;break;case 59:key=SDLK_SEMICOLON;break;case 61:key=SDLK_EQUALS;break;case 91:key=SDLK_LEFTBRACKET;break;case 92:key=SDLK_BACKSLASH;break;case 93:key=SDLK_RIGHTBRACKET;break;case 192:key=SDLK_BACKQUOTE;break;case 222:key=SDLK_QUOTE;break;default:known=false;break;}}

    }
    if(known){tr_sdl_case(op,key,mods,expected);if(ch>='A' && ch<='Z' && (vk==0 || vk==65))tr_sdl_case(op,ch+32,KMOD_CAPS,expected);if(ch>='a' && ch<='z' && vk==0)tr_sdl_case(op,ch,(SDL_Keymod)(KMOD_SHIFT|KMOD_CAPS),expected);}
    if(vk==0 && ch==0)for(int letter='a';letter<='z';letter++)tr_sdl_case(op,letter,KMOD_CTRL,expected);
    if(vk==0 && ch==30)tr_sdl_case(op,'6',(SDL_Keymod)(KMOD_CTRL|KMOD_SHIFT),expected);
    if(vk==0 && ch==31)tr_sdl_case(op,'-',(SDL_Keymod)(KMOD_CTRL|KMOD_SHIFT),expected);
}
static void tracking_fault_controls(void){
    InputTracking t={0};input_tracking_enable(&t,100000);tr_check(input_tracking_event(&t,TRACK_KEY_PRESS,0,0,65,100010),"fault seed");InputTracking before=t;int guards=0;
    for(int kind=-1;kind<=15;kind++)if(kind!=1 && kind!=3 && kind!=5 && !(kind>=8 && kind<=13)){tr_check(!input_tracking_event(&t,kind,0,0,0,100020) && !memcmp(&t,&before,sizeof(t)),"invalid event kind retains state");guards++;}
    tr_check(!input_tracking_event(NULL,1,0,0,0,0) && input_tracking_take(NULL,false,NULL,0)==-2,"null tracker guards");guards+=2;
    uint8_t bytes[500];memset(bytes,0xA5,sizeof(bytes));tr_check(input_tracking_take(&t,true,bytes,2)==-2 && !memcmp(&t,&before,sizeof(t)) && bytes[0]==0xA5,"take capacity staged");tr_check(input_tracking_take(&t,true,NULL,500)==-2 && !memcmp(&t,&before,sizeof(t)),"null take destination");guards+=2;
    for(int field=0;field<3;field++){t=before;if(field==0)t.out_length=500;if(field==1)t.old_length=500;if(field==2)t.out_present=false;InputTracking bad=t;tr_check(!input_tracking_event(&t,1,1,1,0,0) && input_tracking_take(&t,true,bytes,500)==-2 && !memcmp(&t,&bad,sizeof(t)),"invalid buffer metadata retained");guards++;}t=before;
    GameSession *session=calloc(1,sizeof(*session));tr_check(session!=NULL,"fault session");protocol_init_session(session);session->in_game=true;session->stream.buffered=true;uint32_t seed[]={225,81,133,226};isaac_init(&session->out_cipher,seed,4);tr_check(protocol_tracking_control(session,&t,999,NULL,99,0)==0 && protocol_tracking_control(session,&t,226,NULL,1,0)==-1 && protocol_tracking_control(session,&t,133,NULL,1,0)==-1 && protocol_tracking_control(NULL,&t,133,NULL,0,0)==-1 && protocol_tracking_control(session,NULL,226,NULL,0,0)==-1 && !memcmp(&t,&before,sizeof(t)),"control opcode/length/null guards");guards+=5;
    tr_check(!protocol_send_tracking(session,NULL,1) && !protocol_send_tracking(session,bytes,501) && !protocol_send_tracking(NULL,bytes,1),"sender argument guards");guards+=3;
    t.out_length=498;memset(t.out,0x5A,498);tr_check(input_tracking_event(&t,TRACK_KEY_PRESS,0,0,65,100030) && t.old_present && t.old_length==498 && t.out_length==3,"rollover seed");before=t;session->stream.out_len=4500;memset(session->stream.out_buf,0xA5,5000);Isaac cipher=session->out_cipher;tr_check(!protocol_flush_tracking(session,&t) && session->stream.out_overflow && session->stream.out_len==4500 && !memcmp(&t,&before,sizeof(t)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)),"flush overflow retains payload andcipher");for(int i=0;i<5000;i++)tr_check(session->stream.out_buf[i]==0xA5,"overflow bytes retained");session->stream.out_len=0;tr_check(!protocol_flush_tracking(session,&t) && !memcmp(&t,&before,sizeof(t)),"sticky overflow retains payload");session->stream.out_overflow=false;session->stream.buffered=false;session->stream.connected=false;tr_check(!protocol_flush_tracking(session,&t) && !memcmp(&t,&before,sizeof(t)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)),"disconnected flush retains payload andcipher");session->stream.buffered=true;session->stream.out_len=4499;tr_check(protocol_flush_tracking(session,&t) && session->stream.out_len==5000 && !t.old_present && t.out_length==3,"exact flush capacity recovery");
    before=t;cipher=session->out_cipher;tr_check(protocol_tracking_control(session,&t,133,NULL,0,0)==-2 && !memcmp(&before,&t,sizeof(t)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)),"stop overflow retains recorder");session->stream.out_overflow=false;session->stream.out_len=4994;tr_check(protocol_tracking_control(session,&t,133,NULL,0,0)==1 && session->stream.out_len==5000 && !t.enabled && !t.out_present && !t.old_present,"exact stop recovery");session->stream.out_len=4497;memset(bytes,0xCC,sizeof(bytes));tr_check(protocol_send_tracking(session,bytes,500) && session->stream.out_len==5000 && session->stream.out_buf[4498]==1 && session->stream.out_buf[4499]==244 && !memcmp(session->stream.out_buf+4500,bytes,500),"maximum tracking body framing");
    SDL_Event event;memset(&event,0,sizeof(event));event.type=SDL_WINDOWEVENT;const Uint8 windows[]={SDL_WINDOWEVENT_FOCUS_GAINED,SDL_WINDOWEVENT_FOCUS_LOST,SDL_WINDOWEVENT_ENTER,SDL_WINDOWEVENT_LEAVE};const ClientEventType types[]={CLIENT_EVENT_FOCUS_GAIN,CLIENT_EVENT_FOCUS_LOSS,CLIENT_EVENT_MOUSE_ENTER,CLIENT_EVENT_MOUSE_EXIT};for(int i=0;i<4;i++){ClientEvent out;event.window.event=windows[i];tr_check(tracking_sdl_event(&event,&out) && out.type==types[i],"SDL focus/window translation");}ClientEvent out;event.window.event=SDL_WINDOWEVENT_RESIZED;tr_check(!tracking_sdl_event(&event,&out),"resize untracked");event.type=SDL_TEXTINPUT;tr_check(!tracking_sdl_event(&event,&out),"typed text not duplicate key tracking");event.type=SDL_MOUSEWHEEL;tr_check(!tracking_sdl_event(&event,&out),"wheel not invented in225 tracking");
    free(session);printf("PASS tracking failures:%d invalid-kind/null/metadata/length guards;flush/stop capacity,sticky anddisconnected state/cipher retention,exact recovery,500-byte framing;4 SDL focus/window routes and3 untracked types\n",guards);
}
static void tr_native_action(InputTracking *t,GameSession *session,int op,int x,int y,int code,int64_t now){
    if(op==16)input_tracking_init(t);
    else if(op==226 || op==133)tr_check(protocol_tracking_control(session,t,op,NULL,0,now)==1,"native control");
    else if(op==14)tr_check(protocol_flush_tracking(session,t),"native flush");
    else if(op==0)input_tracking_disable(t);
    else if(op==18)t->tracked_count=x;
    else {int kind=op,value=code==1?1:0;if(op==6)kind=TRACK_MOVE;if(op==8 || op==9)value=input_tracking_shell_key(x,y,op==8);if(op==17){kind=code?TRACK_KEY_RELEASE:TRACK_KEY_PRESS;value=x;}tr_check(input_tracking_event(t,kind,x,y,value,now),"native input event");}
}
static void tracking_reference_run(const char *path,InputTracking *t,GameSession *session,const TrackingHooks *hooks){
    FILE *f=fopen(path,"rb");tr_check(f && tr_u32(f)==0x49543235,"original fixture");int count=(int)tr_u32(f);tr_check(count==5339,"case count");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=tr_u32(f);session->in_game=true;session->stream.buffered=true;int controls=0,flushes=0;tr_sdl_checks=0;
    for(tr_record=0;tr_record<count;tr_record++){
        int op=(int32_t)tr_u32(f),x=(int32_t)tr_u32(f),y=(int32_t)tr_u32(f),code=(int32_t)tr_u32(f);int64_t now=tr_i64(f);session->stream.out_len=0;if(op==16)isaac_init(&session->out_cipher,seed,4);
        int32_t before_count=t->tracked_count;if(hooks && hooks->action)hooks->action(op,x,y,code,now);else tr_native_action(t,session,op,x,y,code,now);if(op==226 || op==133)controls++;if(op==14)flushes++;
        tr_check(tr_bool(f)==t->enabled,"enabled");tr_equal(f,t->tracked_count,"tracked count");tr_check(tr_i64(f)==t->last_time,"last time");tr_check(tr_i64(f)==t->last_move_time,"last move time");tr_equal(f,t->last_x,"last x");tr_equal(f,t->last_y,"last y");tr_buffer(f,t->out_present,t->out,t->out_length);tr_buffer(f,t->old_present,t->old,t->old_length);int clocks=(int)tr_u32(f);if(hooks && hooks->clock_calls)tr_check(clocks==hooks->clock_calls(),"source clock call count");
        if((op==8 || op==9) && t->out_present && t->out_length>=3 && t->tracked_count!=before_count)tr_sdl_probe(op,x,y,code,t->out[t->out_length-1]);
        int n=(int)tr_u32(f);uint8_t expected[503];tr_check(n>=0 && n<=503 && fread(expected,1,(size_t)n,f)==(size_t)n,"wire bytes");if(hooks && hooks->wire)hooks->wire(session,n);tr_check(session->stream.out_len==(size_t)n && !memcmp(session->stream.out_buf,expected,(size_t)n),"encrypted81 framing/body");tr_cipher(f,&session->out_cipher);
    }
    tr_check(fgetc(f)==EOF && controls==336 && flushes==163,"fixture tail/counts");fclose(f);printf("PASS original tracking:%d states,%d read133/226 controls,%d updateGame flush prefixes;all buffers/count/time/position/wire/full ISAAC;GameShell keys/mouse/focus,rollover/overwrite/stop quirks,signed-time andcount wrap;%d SDL key translations vs original bytes\n",count,controls,flushes,tr_sdl_checks);
}
static void tracking_registered_reference(void){InputTracking tracking={0};GameSession *session=calloc(1,sizeof(*session));tr_check(session!=NULL,"registered session");protocol_init_session(session);const char *path=getenv("RS2_TRACKING_ORACLE");tracking_reference_run(path && *path?path:"tests/fixtures/input-tracking-reference.bin",&tracking,session,NULL);free(session);tracking_fault_controls();}
#endif
