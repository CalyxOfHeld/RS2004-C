#include "net/protocol.h"
#include <assert.h>
static bool tracking_login_injected;
static LoginResponse tracking_login_reply;
static LoginResponse tracking_login(GameSession *session,const char *host,int port,const char *name,const char *password,bool low,bool reconnect){
    (void)host;(void)port;(void)name;(void)password;(void)low;(void)reconnect;assert(tracking_login_injected);session->in_game=true;session->is_mod=tracking_login_reply==LOGIN_MOD_SUCCESS;return tracking_login_reply;
}
#define protocol_login_ex tracking_login
#include "../../../src/client/client.c"
#undef protocol_login_ex
#include "client/client_log.h"
#include "input_tracking_reference_control.h"
#include <process.h>
#include <direct.h>
static socket_t tracking_peer;
static Isaac tracking_sender;
static int64_t tracking_now;
static int tracking_clocks,tracking_packets,tracking_ticks,tracking_events;
static uint64_t tracking_time_raw(void){return (uint64_t)tracking_now;}
static uint64_t tracking_time(void){tracking_clocks++;return client_log_time(tracking_time_raw);}
static int tracking_clock_calls(void){return tracking_clocks;}
static void tracking_pin_tick(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;}
static void tracking_wire(GameSession *s,int count){s->stream.out_len=0;while(s->stream.out_len<(size_t)count){int n=recv(tracking_peer,(char*)s->stream.out_buf+s->stream.out_len,count-(int)s->stream.out_len,0);tr_check(n>0,"candidate outgoing bytes");s->stream.out_len+=(size_t)n;}fd_set ready;FD_ZERO(&ready);FD_SET(tracking_peer,&ready);struct timeval timeout={0,0};tr_check(select(0,&ready,NULL,NULL,&timeout)==0,"unexpected candidate wire");}
static void tracking_control(const uint8_t *opcodes,int count){
    uint8_t bytes[8];tr_check(count>0 && count<=8,"control batch count");for(int i=0;i<count;i++)bytes[i]=(uint8_t)(opcodes[i]+isaac_next(&tracking_sender));tr_check(send(tracking_peer,(char*)bytes,count,0)==count && client_stream_wait_readable(&session.stream,1000)==1,"candidate control send");tracking_pin_tick();tr_check(client_update() && ingame && !session.protocol_error && !reconnecting,"candidate update");tr_check(!memcmp(&session.in_cipher,&tracking_sender,sizeof(tracking_sender)),"full incoming cipher");tracking_packets+=count;tracking_ticks++;
}
static void tracking_action(int op,int x,int y,int code,int64_t now){
    tracking_now=now;
    if(op==16){input_tracking_init(&input_tracking);tracking_clocks=0;}
    else if(op==226 || op==133){uint8_t opcode=(uint8_t)op;tracking_control(&opcode,1);}
    else if(op==14){tracking_pin_tick();tr_check(client_update() && ingame && !session.protocol_error && !reconnecting,"actual updateGame flush phase");tracking_ticks++;}
    else if(op==0)input_tracking_disable(&input_tracking);
    else if(op==18)input_tracking.tracked_count=x;
    else {
        ClientEvent event={0};event.x=x;event.y=y;
        if(op==1 || op==3){event.type=op==1?CLIENT_EVENT_MOUSE_DOWN:CLIENT_EVENT_MOUSE_UP;event.button=code==1?CLIENT_BUTTON_RIGHT:CLIENT_BUTTON_LEFT;}
        else if(op==5 || op==6)event.type=CLIENT_EVENT_MOUSE_MOVE;
        else if(op==8 || op==9 || op==17){event.type=(op==8 || (op==17 && !code))?CLIENT_EVENT_TRACK_KEY_DOWN:CLIENT_EVENT_TRACK_KEY_UP;event.key=op==17?x:input_tracking_shell_key(x,y,op==8);}
        else if(op==10)event.type=CLIENT_EVENT_FOCUS_GAIN;else if(op==11)event.type=CLIENT_EVENT_FOCUS_LOSS;else if(op==12)event.type=CLIENT_EVENT_MOUSE_ENTER;else if(op==13)event.type=CLIENT_EVENT_MOUSE_EXIT;else tr_check(false,"candidate event opcode");
        client_queue_event(&event);event_read=event_write;tracking_events++;
    }
}
static void tracking_log_roundtrip(const char *path){
    InputTracking *trace=malloc(480*sizeof(*trace));tr_check(trace!=NULL,"per-event replay states");FILE *existing=fopen(path,"rb");tr_check(existing==NULL,"fresh event log");tr_check(session_log_record(path),"event log record");input_tracking_init(&input_tracking);tracking_clocks=0;tracking_now=1000000;input_tracking_enable(&input_tracking,(int64_t)tracking_time());
    for(int i=0;i<480;i++){
        ClientEvent event={0};event.type=(ClientEventType)(CLIENT_EVENT_TRACK_KEY_DOWN+i%6);event.key=i%2?65535:'A';event.x=i;event.y=479-i;snprintf(event.text,sizeof(event.text),"synthetic-%d",i);tracking_now=i==241?INT64_MIN+10:1000000+i*51;client_log_event(&event);client_queue_event(&event);trace[i]=input_tracking;
    }
    InputTracking expected=input_tracking;int clocks=tracking_clocks;session_log_close();tr_check(session_log_replay(path),"event log replay");input_tracking_init(&input_tracking);tracking_clocks=0;tracking_now=-9999;input_tracking_enable(&input_tracking,(int64_t)tracking_time());int count=0;while(session_log_peek() && !strcmp(session_log_peek(),"E")){ClientEvent event;tr_check(client_log_take_event(&event),"recorded tracking event");client_queue_event(&event);tr_check(count<480 && !memcmp(&input_tracking,&trace[count],sizeof(input_tracking)) && tracking_clocks==count+2,"per-event recorder/clock replay state");count++;}
    tr_check(count==480 && tracking_clocks==clocks && !memcmp(&input_tracking,&expected,sizeof(expected)) && !session_log_failed() && session_log_at_end(),"replayed complete recorder and clock state");session_log_close();free(trace);printf("PASS tracking event log:480 per-event states from new key/focus/window events and%d clock reads reproduce complete recorder/buffers;recorded negative clock overrides live clock;RS2REC event fields unchanged\n",clocks);
}
static void tracking_lifecycle_controls(void){
    session.stream.out_len=0;tracking_now=2000000;uint8_t both[]={226,226};int clocks=tracking_clocks;tracking_control(both,2);tracking_wire(&session,0);tr_check(input_tracking.enabled && input_tracking.out_length==0 && !input_tracking.old_present && tracking_clocks==clocks+2,"coalesced repeated enable");uint8_t empty_stop[]={226,133};tracking_control(empty_stop,2);tracking_wire(&session,0);tr_check(!input_tracking.enabled,"coalesced enable/empty stop");
    for(int reply=LOGIN_SUCCESS;reply<=LOGIN_MOD_SUCCESS;reply+=16){input_tracking_enable(&input_tracking,2000000);tr_check(input_tracking_event(&input_tracking,TRACK_KEY_PRESS,0,0,'Z',2000010),"login recorder seed");InputTracking before=input_tracking;tracking_login_injected=true;tracking_login_reply=(LoginResponse)reply;puts("[TRACK CONTROL] Injected native login reply;no authentication connection/account used");client_login(false);tracking_login_injected=false;tr_check(!input_tracking.enabled && !input_tracking.out_present && !input_tracking.old_present && input_tracking.tracked_count==before.tracked_count && input_tracking.last_time==before.last_time && input_tracking.last_move_time==before.last_move_time && input_tracking.last_x==before.last_x && input_tracking.last_y==before.last_y,"normal/mod login disables and retains source metadata");}
    input_tracking_enable(&input_tracking,2000100);tr_check(input_tracking_event(&input_tracking,TRACK_KEY_RELEASE,0,0,'Z',2000110),"reconnect recorder seed");InputTracking before=input_tracking;tracking_login_injected=true;tracking_login_reply=LOGIN_RECONNECT_SUCCESS;puts("[TRACK CONTROL] Injected native reconnect15 reply;no authentication connection/account used");client_login(true);tracking_login_injected=false;tr_check(!memcmp(&input_tracking,&before,sizeof(before)),"reconnect15 retains recorder");
    uint8_t logout=(uint8_t)(142+isaac_next(&tracking_sender));session.stream.out_len=0;tr_check(send(tracking_peer,(char*)&logout,1,0)==1 && client_stream_wait_readable(&session.stream,1000)==1,"server logout control");tracking_pin_tick();tr_check(client_update() && !ingame && !input_tracking.enabled && !input_tracking.out_present && !input_tracking.old_present && input_tracking.tracked_count==before.tracked_count && input_tracking.last_time==before.last_time,"actual server logout disables recorder");uint8_t byte;tr_check(recv(tracking_peer,(char*)&byte,1,0)==0,"logout closes without tracking report");puts("PASS tracking lifecycle:coalesced226/226 and226/133,actual native normal2/mod18 disable,reconnect15 retention andserver142 logout;login replies injected only;no credentials/production connection");
}
static void tracking_duplicate_controls(void){
    input_tracking_init(&input_tracking);input_tracking_enable(&input_tracking,2000000);tracking_now=2000010;int clocks=tracking_clocks;SDL_Event sdl;memset(&sdl,0,sizeof(sdl));sdl.type=SDL_KEYDOWN;sdl.key.keysym.sym=SDLK_a;sdl.key.keysym.mod=KMOD_SHIFT;ClientEvent event;tr_check(tracking_sdl_event(&sdl,&event),"raw SDL press");client_queue_event(&event);event=(ClientEvent){.type=CLIENT_EVENT_KEY_DOWN,.key=CLIENT_KEY_NONE};client_queue_event(&event);event=(ClientEvent){.type=CLIENT_EVENT_TEXT,.text="A"};client_queue_event(&event);event_read=event_write;tr_check(input_tracking.tracked_count==1 && input_tracking.out_length==3 && input_tracking.out[2]=='A' && tracking_clocks==clocks+1,"normal key/text do not double-record raw press");
    sdl.type=SDL_KEYUP;tr_check(tracking_sdl_event(&sdl,&event),"raw SDL release");client_queue_event(&event);event=(ClientEvent){.type=CLIENT_EVENT_KEY_UP};client_queue_event(&event);event_read=event_write;tr_check(input_tracking.tracked_count==2 && input_tracking.out_length==6 && input_tracking.out[3]==9 && input_tracking.out[5]=='A' && tracking_clocks==clocks+2,"normal release does not double-record raw release");puts("PASS tracking duplicate control:actual SDL raw press/release plusnormal key/text events produceexactly2 records/2 clock reads");
}
int main(int argc,char **argv){
    setvbuf(stdout,NULL,_IONBF,0);tr_check(argc==3,"candidate fixture/log arguments");(void)tracking_registered_reference;ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};platform.tracking_now_ms=tracking_time;tr_check(client_init(&options,&platform) && client_stream_global_init(),"candidate real-cache init");socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);tr_check(listener!=INVALID_SOCKET_VAL,"listener");struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);tr_check(!bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"loopback listen");int size=sizeof(address);tr_check(!getsockname(listener,(struct sockaddr*)&address,&size),"port");protocol_init_session(&session);tr_check(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"isolated connect");tracking_peer=accept(listener,NULL,NULL);tr_check(tracking_peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);DWORD timeout=3000;setsockopt(tracking_peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));uint32_t seed[]={225,226,133,81};isaac_init(&session.in_cipher,seed,4);isaac_init(&tracking_sender,seed,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
    TrackingHooks hooks={tracking_action,tracking_wire,tracking_clock_calls};tracking_reference_run(argv[1],&input_tracking,&session,&hooks);tr_check(tracking_packets==336 && tracking_ticks==499,"candidate primary packet/tick coverage");printf("PASS candidate tracking:5339 original states,336 ISAAC-framed read controls,499 actual client_update calls,%d synthetic queue events;full source clock-call counts/buffers/cipher/wire;UI consumption held at observer boundary\n",tracking_events);tracking_log_roundtrip(argv[2]);tracking_duplicate_controls();tracking_lifecycle_controls();tracking_fault_controls();closesocket(tracking_peer);live_server=false;client_shutdown();client_stream_global_cleanup();return 0;
}
