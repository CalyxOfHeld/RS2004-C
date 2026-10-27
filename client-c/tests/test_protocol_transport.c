#include "net/protocol.h"
#include "../verification/completion/tools/input_tracking_reference_control.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

static socket_t peer;
static GameSession session;
static Isaac encoder, outgoing_decoder;
static const uint32_t seed[4]={17,31,57,99};
static uint64_t millis(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timeval tv; gettimeofday(&tv,NULL); return (uint64_t)tv.tv_sec*1000+tv.tv_usec/1000;
#endif
}
static void close_socket(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}
static void connect_pair(void) {
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); assert(listener!=INVALID_SOCKET_VAL);
    struct sockaddr_in address={0}; address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=0;
    assert(bind(listener,(struct sockaddr *)&address,sizeof(address))==0);
    assert(listen(listener,1)==0);
#ifdef _WIN32
    int size=sizeof(address);
#else
    socklen_t size=sizeof(address);
#endif
    assert(getsockname(listener,(struct sockaddr *)&address,&size)==0);
    protocol_init_session(&session);
    assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)));
    peer=accept(listener,NULL,NULL); assert(peer!=INVALID_SOCKET_VAL); close_socket(listener);
    session.in_game=true;
    isaac_init(&session.in_cipher,seed,4); isaac_init(&encoder,seed,4);
    isaac_init(&session.out_cipher,seed,4); isaac_init(&outgoing_decoder,seed,4);
}
static void disconnect_pair(void) { protocol_close_session(&session); if(peer!=INVALID_SOCKET_VAL) close_socket(peer); peer=INVALID_SOCKET_VAL; }
static void transmit(const uint8_t *bytes,int length) { assert(send(peer,(const char *)bytes,length,0)==length); }
static uint8_t encrypted(int opcode) { return (uint8_t)(opcode+isaac_next(&encoder)); }
static int poll(uint8_t *out,size_t capacity,size_t *length,int *opcode) {
    return protocol_poll_packet(&session,opcode,out,capacity,length);
}
static int poll_ready(uint8_t *out,size_t capacity,size_t *length,int *opcode) {
    assert(client_stream_wait_readable(&session.stream,1000)==1); return poll(out,capacity,length,opcode);
}
static void receive_peer(uint8_t *out,int size) {
    int count=0; while(count<size) {int n=recv(peer,(char *)out+count,size-count,0); assert(n>0);count+=n;}
    out[0]=(uint8_t)(out[0]-isaac_next(&outgoing_decoder));
}
int main(void) {
    assert(client_stream_global_init());
    uint8_t out[32]; size_t length; int opcode;
    connect_pair();
    for (int cycle=1;cycle<=750;cycle++) assert(!protocol_tick_network_idle(&session));
    assert(session.idle_net_cycles==750);
    assert(protocol_tick_network_idle(&session));
    assert(protocol_tick_network_idle(&session) && session.idle_net_cycles==751);
    memset(out,0xa5,sizeof(out));
    uint8_t header[]={encrypted(98),0,5};
    transmit(header,1); uint64_t start=millis();
    assert(poll_ready(out,sizeof(out),&length,&opcode)==0); assert(millis()-start<250);
    transmit(header+1,1); assert(poll_ready(out,sizeof(out),&length,&opcode)==0);
    transmit(header+2,1); assert(poll_ready(out,sizeof(out),&length,&opcode)==0);
    assert(session.idle_net_cycles==751); /* partial header cannot reset silence */
    const uint8_t payload[]={1,2,3,4,5};
    transmit(payload,2); assert(poll_ready(out,sizeof(out),&length,&opcode)==0);
    for(size_t i=0;i<sizeof(out);i++) assert(out[i]==0xa5);
    start=millis(); for(int i=0;i<100;i++) assert(poll(out,sizeof(out),&length,&opcode)==0);
    assert(millis()-start<250);
    transmit(payload+2,3); assert(poll_ready(out,sizeof(out),&length,&opcode)==1);
    assert(opcode==98 && length==5 && !memcmp(out,payload,5));
    assert(session.idle_net_cycles==0);
    assert(!protocol_tick_network_idle(&session) && session.idle_net_cycles==1);
    puts("PASS: original receive watchdog boundary 750/751; fragments retain timer, complete packet resets");
    puts("PASS NET-01: split opcode, two-byte length and payload; no output mutation or blocking while incomplete");
    uint8_t batch[]={encrypted(239),encrypted(150),0,43,2,encrypted(4),3,'o','k',10,encrypted(223),1,2,3,4,5};
    transmit(batch,sizeof(batch));
    assert(poll_ready(out,sizeof(out),&length,&opcode)==1 && opcode==239 && length==0);
    assert(poll(out,sizeof(out),&length,&opcode)==1 && opcode==150 && length==3 && out[2]==2);
    assert(poll(out,sizeof(out),&length,&opcode)==1 && opcode==4 && length==3 && out[2]==10);
    assert(poll(out,sizeof(out),&length,&opcode)==1 && opcode==223 && length==5 && out[4]==5);
    puts("PASS NET-02: coalesced empty/fixed/byte-length/zone packets preserve ISAAC and framing");
    disconnect_pair(); connect_pair();
    uint8_t oversize[]={encrypted(98),0,33}; transmit(oversize,3); memset(out,0xa5,sizeof(out));
    assert(poll_ready(out,sizeof(out),&length,&opcode)==-1 && !session.in_game && session.protocol_error);
    for(size_t i=0;i<sizeof(out);i++) assert(out[i]==0xa5);
    disconnect_pair();connect_pair();
    uint8_t unknown=encrypted(62);transmit(&unknown,1);
    assert(poll_ready(out,sizeof(out),&length,&opcode)==-1 && !session.in_game && session.protocol_error);
    disconnect_pair();connect_pair();
    uint8_t incomplete[]={encrypted(98),0,5,1,2};transmit(incomplete,sizeof(incomplete));
    assert(poll_ready(out,sizeof(out),&length,&opcode)==0);
    close_socket(peer);peer=INVALID_SOCKET_VAL;
    assert(poll_ready(out,sizeof(out),&length,&opcode)==-1 && !session.in_game && !session.protocol_error);
    disconnect_pair();
    puts("PASS NET-03: oversize, unknown opcode and mid-payload close fail explicitly with protected output");
    connect_pair(); uint8_t wire[128];
    assert(protocol_send_opheld(&session,2,1277,17,3214));receive_peer(wire,7);
    assert(wire[0]==71 && wire[1]==4 && wire[2]==253 && wire[3]==0 && wire[4]==17 && wire[5]==12 && wire[6]==142);
    assert(protocol_send_inv_button(&session,1,995,9,5382));receive_peer(wire,7);
    assert(wire[0]==31 && wire[3]==0 && wire[4]==9 && wire[5]==21 && wire[6]==6);
    int8_t rx[]={2,2},rz[]={0,3};
    assert(protocol_send_walk(&session,3200,3200,2,rx,rz,false));receive_peer(wire,11);
    assert(wire[0]==181 && wire[1]==9 && wire[3]==12 && wire[4]==128 && wire[7]==2 && wire[10]==3);
    assert(!protocol_send_walk(&session,0,0,25,rx,rz,false));
    assert(!protocol_send_walk(&session,0,0,-1,rx,rz,false));
    int route_x[]={48,49,50,50,50},route_z[]={48,48,48,49,50};
    assert(protocol_send_route(&session,3184,3184,47,48,route_x,route_z,5,false));receive_peer(wire,9);
    const uint8_t route_wire[]={181,7,0,12,162,12,160,0,2};
    assert(!memcmp(wire,route_wire,sizeof(route_wire)));
    assert(!protocol_send_route(&session,3184,3184,47,48,NULL,route_z,5,false));
    assert(protocol_send_operation_route(&session,3184,3184,47,48,route_x,route_z,5,false));receive_peer(wire,9);
    assert(wire[0]==93 && !memcmp(wire+1,route_wire+1,8));
    assert(protocol_send_opnpc(&session,1,5124));receive_peer(wire,3);assert(wire[0]==194 && wire[1]==20 && wire[2]==4);
    assert(protocol_send_oploc(&session,1,3230,3208,1530));receive_peer(wire,7);
    const uint8_t loc_wire[]={245,12,158,12,136,5,250};assert(!memcmp(wire,loc_wire,7));
    assert(protocol_send_opplayer(&session,3,15));receive_peer(wire,3);assert(wire[0]==185 && wire[2]==15);
    puts("PASS: reference turn-first route compression, world base/start-relative offsets, operation movement and NPC/loc/player operation bytes");
    assert(protocol_send_command(&session,"test"));receive_peer(wire,7);
    assert(wire[0]==4 && wire[1]==5 && !memcmp(wire+2,"test\n",5));
    assert(protocol_send_keepalive(&session));receive_peer(wire,1);assert(wire[0]==108);
    puts("PASS: real outgoing socket bytes match bundled operation layouts, waypoint limit, string delimiter and NO_TIMEOUT");
    disconnect_pair();tracking_registered_reference();client_stream_global_cleanup();
    puts("[Test Protocol Transport] PASS (loopback transport/codec checks; not live gameplay or visual parity)");
    return 0;
}
