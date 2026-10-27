#include "net/protocol.h"
#include <windows.h>
#include <string.h>
static int WSAAPI continue_lookup(const struct sockaddr *a,socklen_t b,char *c,DWORD d,char *e,DWORD f,int g){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;return WSAHOST_NOT_FOUND;}
static LoginResponse continue_reply;
static LoginResponse continue_login(GameSession *s,const char *host,int port,const char *name,const char *password,bool low,bool reconnect){(void)host;(void)port;(void)name;(void)password;(void)low;(void)reconnect;s->in_game=true;s->is_mod=continue_reply==LOGIN_MOD_SUCCESS;return continue_reply;}
#define protocol_login_ex continue_login
#define getnameinfo continue_lookup
#include "../../../src/client/client.c"
#undef getnameinfo
#undef protocol_login_ex
#include "continue_reference_control.h"
static socket_t ct_peer;
static Isaac ct_sender;
static int ct_packets,ct_pictures,ct_fragments;
static size_t ct_foreground;
static void ct_pin(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;}
static void ct_action(int op,int id){actions.live=true;client_dispatch_action((MenuAction){.type=op?MENU_IF_CLOSE:MENU_IF_CONTINUE,.component=id});}
static void ct_packet(int op,const uint8_t *payload,size_t n){
 uint8_t frame[10];frame[0]=(uint8_t)(op+isaac_next(&ct_sender));if(n)memcpy(frame+1,payload,n);ct_pin();
 if(ct_fragments<18 && n){ct_check(send(ct_peer,(char*)frame,1,0)==1,"fragment header");ct_check(client_stream_wait_readable(&session.stream,1000)==1,"fragment ready");bool waiting=comp_cfg.pressed_continue;ct_check(client_update() && comp_cfg.pressed_continue==waiting,"partial payload retains waiting");ct_check(send(ct_peer,(char*)frame+1,(int)n,0)==(int)n,"fragment payload");ct_fragments++;}
 else ct_check(send(ct_peer,(char*)frame,(int)n+1,0)==(int)n+1,"framed packet send");
 ct_pin();ct_check(client_stream_wait_readable(&session.stream,1000)==1 && client_update() && ingame && !reconnecting && !session.protocol_error,"actual candidate packet update");ct_check(!memcmp(&session.in_cipher,&ct_sender,sizeof(ct_sender)),"full incoming ISAAC");ct_packets++;
}
static void ct_wire(GameSession *s,int n){
 ct_check(client_stream_flush(&s->stream),"flush action output");s->stream.out_len=0;
 while(s->stream.out_len<(size_t)n){int got=recv(ct_peer,(char*)s->stream.out_buf+s->stream.out_len,n-(int)s->stream.out_len,0);ct_check(got>0,"candidate wire read");s->stream.out_len+=(size_t)got;}
 fd_set ready;FD_ZERO(&ready);FD_SET(ct_peer,&ready);struct timeval timeout={0,0};ct_check(select(0,&ready,NULL,NULL,&timeout)==0,"no extra candidate wire");
}
static void ct_picture(Component *root,const uint32_t *expected,int i){
 Component saved=comp_cfg.instances[1];comp_cfg.instances[1]=*root;interfaces.viewport=1;interfaces.sidebar=interfaces.chat=-1;varp_ctx.energy=(i&4)!=0?1:0;streamer.scene_state=2;player.visible=false;menu.visible=false;chatbox.private_prompt_open=chatbox.count_dialog_open=false;mouse_x=mouse_y=-1;
 ct_check(client_draw(),"actual candidate draw");for(int j=0;j<512*334;j++)if(expected[j]){ct_check(pixels[(VIEWPORT_Y+j/512)*CLIENT_WIDTH+VIEWPORT_X+j%512]==expected[j],"candidate original foreground pixel");ct_foreground++;}comp_cfg.instances[1]=saved;ct_pictures++;
}
static void ct_click_controls(void){
 Component saved0=comp_cfg.instances[0],saved1=comp_cfg.instances[1];int id=0,x=12,y=10;
 comp_cfg.instances[0]=(Component){.id=0,.type=4,.button_type=6,.width=180,.height=30,.over_layer=-1,.option="Click here to continue",.text="Click here to continue",.active_text="",.font=&font_p12,.colour=0xffff00};
 comp_cfg.instances[1]=(Component){.id=1,.type=0,.width=512,.height=334,.child_count=1,.child_id=&id,.child_x=&x,.child_y=&y};
 interfaces.viewport=1;interfaces.sidebar=interfaces.chat=-1;comp_cfg.pressed_continue=false;menu.visible=false;session.stream.out_len=0;streamer.scene_state=2;scene_build_pending=false;Isaac expected=session.out_cipher;uint8_t opcode=(uint8_t)(235+isaac_next(&expected));
 for(int i=0;i<3;i++){ClientEvent move={.type=CLIENT_EVENT_MOUSE_MOVE,.x=VIEWPORT_X+20,.y=VIEWPORT_Y+20};client_queue_event(&move);client_process_events();ct_check(client_draw(),"hover builds current interface menu");ClientEvent down={.type=CLIENT_EVENT_MOUSE_DOWN,.button=1,.x=move.x,.y=move.y};client_queue_event(&down);client_process_events();ct_pin();ct_check(client_update() && client_draw(),"queued actual UI click");ct_check(comp_cfg.pressed_continue,"click sets global waiting");}
 ct_wire(&session,3);ct_check(session.stream.out_buf[0]==opcode && session.stream.out_buf[1]==0 && session.stream.out_buf[2]==0 && !memcmp(&expected,&session.out_cipher,sizeof(expected)),"three UI clicks emit one235/full ISAAC");comp_cfg.instances[0]=saved0;comp_cfg.instances[1]=saved1;session.stream.out_len=0;
 puts("PASS candidate continue UI:three queued primary clicks emit one235 and retain waiting;actual menu suppression");
}
static void ct_lifecycle(void){
 for(int reply=LOGIN_SUCCESS;reply<=LOGIN_MOD_SUCCESS;reply+=16){comp_cfg.pressed_continue=true;continue_reply=(LoginResponse)reply;puts("[CONTINUE CONTROL] Injected native login reply;no authentication connection/account");client_login(false);ct_check(!comp_cfg.pressed_continue,"normal/mod login clears waiting");}
 comp_cfg.pressed_continue=true;continue_reply=LOGIN_RECONNECT_SUCCESS;client_login(true);ct_check(comp_cfg.pressed_continue,"reconnect15 retains waiting");
 puts("PASS continue login lifecycle:injected normal2/mod18 clear,reconnect15 retains;no authentication/account used");
}
int main(int argc,char **argv){
 setvbuf(stdout,NULL,_IONBF,0);ct_check(argc==3,"fixture arguments");(void)continue_registered_reference;ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};ct_check(client_init(&options,&platform) && client_stream_global_init(),"real-cache init");
 socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);ct_check(listener!=INVALID_SOCKET_VAL,"listener");struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);ct_check(!bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"isolated listener");int length=sizeof(address);ct_check(!getsockname(listener,(struct sockaddr*)&address,&length),"ephemeral port");protocol_init_session(&session);ct_check(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"candidate connect");ct_peer=accept(listener,NULL,NULL);ct_check(ct_peer!=INVALID_SOCKET_VAL,"peer accept");closesocket(listener);DWORD timeout=3000;setsockopt(ct_peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));uint32_t seed[]={225,44,14,28};isaac_init(&session.in_cipher,seed,4);isaac_init(&ct_sender,seed,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;actions.live=true;player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
 ContinueHooks hooks={ct_action,ct_packet,ct_wire,ct_picture};continue_reference_run(argv[1],argv[2],&comp_cfg,&session,&interfaces,&chatbox,&last_login225,&selection,&hooks);ct_check(ct_packets==432 && ct_pictures==48 && ct_fragments==18,"candidate coverage counts");ct_click_controls();continue_fault_controls(&comp_cfg);ct_lifecycle();closesocket(ct_peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();printf("PASS candidate continue:1134 original states,432 encrypted receive packets,18 fragmented payloads,48 actual panels/%zu source foreground pixels;full incoming/outgoing cipher and source-specific waiting lifecycle;isolated loopback,no external DNS\n",ct_foreground);return 0;
}
