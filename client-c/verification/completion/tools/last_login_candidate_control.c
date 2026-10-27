#include "ui/component_packets.h"
#include "ui/chatbox.h"
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static HANDLE lookup_gate,lookup_entered;
static volatile LONG lookup_calls,lookup_live;
static volatile LONG lookup_first_address,lookup_last_address;
static int lookup_mode;
static bool fail_dns_allocation,fail_dns_thread;
static int WSAAPI controlled_lookup(const struct sockaddr *address,socklen_t size,char *name,DWORD capacity,char *service,DWORD service_capacity,int flags){
    (void)size;LONG ip=(LONG)ntohl(((const struct sockaddr_in*)address)->sin_addr.s_addr);InterlockedExchange(&lookup_last_address,ip);if(!lookup_calls)InterlockedExchange(&lookup_first_address,ip);(void)service;(void)service_capacity;(void)flags;InterlockedIncrement(&lookup_live);InterlockedIncrement(&lookup_calls);SetEvent(lookup_entered);assert(WaitForSingleObject(lookup_gate,INFINITE)==WAIT_OBJECT_0);int mode=lookup_mode;if(!mode){assert(capacity>strlen("resolved.example"));strcpy(name,"resolved.example");}InterlockedDecrement(&lookup_live);return mode?WSAHOST_NOT_FOUND:0;
}
static void *controlled_calloc(size_t count,size_t size){if(fail_dns_allocation){fail_dns_allocation=false;return NULL;}return calloc(count,size);}
static HANDLE WINAPI controlled_create_thread(LPSECURITY_ATTRIBUTES security,SIZE_T stack,LPTHREAD_START_ROUTINE start,LPVOID data,DWORD flags,LPDWORD id){if(fail_dns_thread){fail_dns_thread=false;return NULL;}return CreateThread(security,stack,start,data,flags,id);}
static Chatbox observed_chat;
static int observe_last_login(LastLoginInfo225 *info,InterfaceState *interfaces,Chatbox *chat,ComponentConfig *cfg,GameSession *session,const uint8_t *data,size_t length){int result=last_login_receive225(info,interfaces,chat,cfg,session,data,length);observed_chat=*chat;return result;}
#define last_login_receive225 observe_last_login
#define getnameinfo controlled_lookup
#define calloc controlled_calloc
#define CreateThread controlled_create_thread
#include "../../../src/client/client.c"
#undef last_login_receive225
#undef getnameinfo
#undef calloc
#undef CreateThread
#include "last_login_reference_control.h"
static socket_t ll_peer;
static Isaac ll_sender;
static int ll_packets,ll_fragments,ll_pictures;
static size_t ll_foreground;
static void ll_pin_tick(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;}
static void ll_packet(const uint8_t *payload,size_t length){
    uint8_t frame[10];ll_require(length==9,"candidate input length");frame[0]=(uint8_t)(140+isaac_next(&ll_sender));memcpy(frame+1,payload,9);ll_pin_tick();
    if(ll_packets%101==0){ll_require(send(ll_peer,(char*)frame,4,0)==4 && client_stream_wait_readable(&session.stream,1000)==1,"fragment send");LastLoginInfo225 before=last_login225;ll_require(client_update() && !memcmp(&before,&last_login225,sizeof(before)),"fragment has no application");ll_require(send(ll_peer,(char*)frame+4,6,0)==6,"fragment rest");ll_fragments++;}
    else ll_require(send(ll_peer,(char*)frame,10,0)==10,"candidate send");
    ll_pin_tick();ll_require(client_stream_wait_readable(&session.stream,1000)==1 && client_update() && ingame && !session.protocol_error && !reconnecting,"candidate packet update");ll_require(!memcmp(&session.in_cipher,&ll_sender,sizeof(ll_sender)),"candidate incoming cipher");chatbox=observed_chat;ll_packets++;
}
static void ll_wire(GameSession *s,int length){s->stream.out_len=0;while(s->stream.out_len<(size_t)length){int n=recv(ll_peer,(char*)s->stream.out_buf+s->stream.out_len,length-(int)s->stream.out_len,0);ll_require(n>0,"outgoing read");s->stream.out_len+=(size_t)n;}fd_set ready;FD_ZERO(&ready);FD_SET(ll_peer,&ready);struct timeval timeout={0,0};ll_require(select(0,&ready,NULL,NULL,&timeout)==0,"unexpected outgoing bytes");}
static void ll_picture(int root,const uint32_t *expected,int picture){
    interfaces.viewport=root;streamer.scene_state=2;player.visible=false;chatbox.private_prompt_open=chatbox.count_dialog_open=false;menu.visible=false;ll_require(client_draw(),"actual candidate draw");for(int i=0;i<512*334;i++)if(expected[i]){uint32_t actual=pixels[(VIEWPORT_Y+i/512)*CLIENT_WIDTH+VIEWPORT_X+i%512];if(actual!=expected[i]){fprintf(stderr,"candidate login panel%d pixel%d actual%x expected%x\n",picture,i,actual,expected[i]);exit(3);}ll_foreground++;}ll_pictures++;
}
static void ll_dns_wait(void){ll_require(login_dns225!=NULL,"DNS worker exists");ll_require(WaitForSingleObject(login_dns225->thread,3000)==WAIT_OBJECT_0,"DNS worker completion");}
static void ll_dns_stage(FILE *f){
    ll_text(f,last_login225.dns);int n=ll_int(f);if(n>=0){char request[16];ll_require(n<16 && fread(request,1,(size_t)n,f)==(size_t)n,"source pending request");request[n]=0;ll_require(login_dns225 && !strcmp(request,last_login225.dns_request),"source pending DNS state");}else ll_require(n==-1 && !login_dns225 && login_dns_started225==last_login225.dns_generation,"source clears pending request");ll_eq(f,(int)lookup_calls,"source resolver calls");uint32_t ip=(uint32_t)lookup_last_address;char address[16];snprintf(address,sizeof(address),"%u.%u.%u.%u",ip>>24,(ip>>16)&255,(ip>>8)&255,ip&255);ll_text(f,address);
}
static void ll_dns_request(const char *address){ResetEvent(lookup_entered);strcpy(last_login225.dns_request,address);strcpy(last_login225.dns,address);last_login225.dns_generation++;client_login_dns_poll225();}
static void ll_dns_controls(const char *source){
    ll_require(WaitForSingleObject(lookup_entered,3000)==WAIT_OBJECT_0,"first lookup entered");ll_require(lookup_first_address==1 && lookup_calls==1 && lookup_live==1 && login_dns225,"bounded worker during corpus requests");SetEvent(lookup_gate);ll_dns_wait();client_login_dns_poll225();ll_require(!login_dns225 && !strcmp(last_login225.dns,"resolved.example") && lookup_calls==1,"source old completion discards newer corpus requests");client_login_dns_clear225();last_login225.dns_generation=0;lookup_calls=lookup_first_address=lookup_last_address=0;
    FILE *f=fopen(source,"rb");ll_require(f && ll_int(f)==0x444e3235 && ll_int(f)==5,"source DNS fixture");
    ResetEvent(lookup_gate);ll_dns_request("192.0.2.1");ll_require(WaitForSingleObject(lookup_entered,3000)==WAIT_OBJECT_0,"source first DNS entered");ll_dns_stage(f);ll_dns_request("198.51.100.2");ll_dns_stage(f);SetEvent(lookup_gate);ll_dns_wait();client_login_dns_poll225();ll_dns_stage(f);
    ll_dns_request("203.0.113.3");ll_dns_wait();client_login_dns_poll225();ll_dns_stage(f);lookup_mode=1;ll_dns_request("203.0.113.4");ll_dns_wait();client_login_dns_poll225();ll_dns_stage(f);ll_require(fgetc(f)==EOF,"source DNS tail");fclose(f);lookup_mode=0;
    last_login225.dns_generation++;strcpy(last_login225.dns,"allocation numeric fallback");fail_dns_allocation=true;client_login_dns_poll225();ll_require(!login_dns225 && !strcmp(last_login225.dns,"allocation numeric fallback"),"lookup allocation fallback");last_login225.dns_generation++;fail_dns_thread=true;client_login_dns_poll225();ll_require(!login_dns225 && !strcmp(last_login225.dns,"allocation numeric fallback"),"lookup thread-create fallback");
    ResetEvent(lookup_gate);ResetEvent(lookup_entered);last_login225.dns_generation++;client_login_dns_poll225();ll_require(WaitForSingleObject(lookup_entered,3000)==WAIT_OBJECT_0 && login_dns225,"shutdown worker entered");HANDLE finished;ll_require(DuplicateHandle(GetCurrentProcess(),login_dns225->thread,GetCurrentProcess(),&finished,SYNCHRONIZE,FALSE,0),"worker observation handle");client_login_dns_clear225();ll_require(!login_dns225,"detached shutdown worker");SetEvent(lookup_gate);ll_require(WaitForSingleObject(finished,3000)==WAIT_OBJECT_0,"detached worker terminates");CloseHandle(finished);ll_require(lookup_live==0 && (uint32_t)lookup_last_address==0xCB007104u,"no live DNS worker/address byte order");
}

int main(int argc,char **argv){
    setvbuf(stdout,NULL,_IONBF,0);ll_require(argc==4,"candidate arguments");(void)last_login_registered_reference;lookup_gate=CreateEventA(NULL,TRUE,FALSE,NULL);lookup_entered=CreateEventA(NULL,TRUE,FALSE,NULL);ll_require(lookup_gate && lookup_entered,"DNS control events");ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};ll_require(client_init(&options,&platform) && client_stream_global_init(),"candidate real-cache init");
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);ll_require(listener!=INVALID_SOCKET_VAL,"listener");struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);ll_require(!bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"loopback listen");int size=sizeof(address);ll_require(!getsockname(listener,(struct sockaddr*)&address,&size),"port");protocol_init_session(&session);ll_require(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"isolated connect");ll_peer=accept(listener,NULL,NULL);ll_require(ll_peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);DWORD timeout=3000;setsockopt(ll_peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));uint32_t seed[]={225,140,9,231};isaac_init(&session.in_cipher,seed,4);isaac_init(&ll_sender,seed,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
    LastLoginHooks225 hooks={ll_packet,ll_wire,ll_picture};last_login_reference_run(argv[1],argv[2],&comp_cfg,&last_login225,&interfaces,&chatbox,&session,&hooks);ll_require(ll_packets==2406 && ll_fragments==24 && ll_pictures==24,"candidate coverage counts");ll_dns_controls(argv[3]);last_login_fault_controls();closesocket(ll_peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();CloseHandle(lookup_gate);CloseHandle(lookup_entered);printf("PASS candidate last-login:2406 ISAAC-framed packet-phase states,678 close231s,24 fragmented payloads,24 actual viewport panels/%zu original foreground pixels;full incoming/outgoing cipher;bounded asynchronous DNS/5 original worker states/old-result-and-request-loss quirk/success/failure/allocation/thread-create/shutdown controls;no external DNS query\n",ll_foreground);return 0;
}
