/* Actual candidate client_update + protocol ISAAC framing on isolated loopback.
 * No login or production server connection; client_init loads real cache. */
#include "../../../src/client/client.c"
#include "core/wordpack.h"
#include <assert.h>
static socket_t peer;
static Isaac sender_cipher;
static void packet_control(int opcode,const uint8_t *payload,size_t length) {
    uint8_t frame[805];size_t n=0;
    frame[n++]=(uint8_t)(opcode+isaac_next(&sender_cipher));
    if(opcode==21)frame[n++]=(uint8_t)(length>>8);
    frame[n++]=(uint8_t)length;if(length)memcpy(frame+n,payload,length);n+=length;
    assert(send(peer,(char*)frame,(int)n,0)==(int)n);
    assert(client_stream_wait_readable(&session.stream,1000)==1);
    assert(client_update());assert(ingame && live_server && !reconnecting && !session.protocol_error);
}
static void message_control(int id,int rights,const char *text) {
    uint8_t bytes[100];Packet p=packet_wrap(bytes,sizeof(bytes));
    p8(&p,(int64_t)jstring_to_base37("alice"));p4(&p,id);p1(&p,rights);
    assert(wordpack_pack(&p,text,strlen(text),true));packet_control(41,bytes,p.pos);
}
int main(void) {
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;
    ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);
    struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);
    int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));
    peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);
    uint32_t seed[]={225,41,21,100};isaac_init(&session.in_cipher,seed,4);isaac_init(&sender_cipher,seed,4);
    session.in_game=true;live_server=true;ingame=true;chatbox_free(&chatbox);chatbox_init(&chatbox,"Self");
    uint8_t ignored[8];Packet p=packet_wrap(ignored,8);p8(&p,(int64_t)jstring_to_base37("alice"));packet_control(21,ignored,8);
    assert(chatbox.ignore_count==1);message_control(1,0,"ignored");assert(chatbox.message_count==0 && chatbox.private_message_next==0);
    message_control(2,2,"fuck");assert(chatbox.message_count==1 && chatbox.type[0]==7 && !strcmp(chatbox.sender[0],"Alice") && !strcmp(chatbox.text[0],"****"));
    message_control(2,2,"duplicate");assert(chatbox.message_count==1 && chatbox.private_message_next==1);
    packet_control(21,NULL,0);message_control(3,0,"Hello WORLD!");
    assert(chatbox.ignore_count==0 && chatbox.message_count==2 && chatbox.type[0]==3 && !strcmp(chatbox.text[0],"Hello world!"));
    /* Original region override blocks even staff messages before ID admission. */
    streamer.base_tile_x=3100;streamer.base_tile_z=3100;player.base.x=0;player.base.z=0;
    assert(client_override_chat225() && !private_override_chat225);
    message_control(4,2,"before draw");assert(chatbox.message_count==3 && chatbox.private_message_next==3);
    streamer.scene_state=2;assert(client_draw());assert(private_override_chat225);
    message_control(5,2,"blocked");assert(chatbox.message_count==3 && chatbox.private_message_next==3);
    closesocket(peer);protocol_close_session(&session);live_server=false;
    client_shutdown();client_stream_global_cleanup();
    puts("PASS candidate private/ignore dispatch:8 ISAAC-framed loopback packets; ignore/moderator/filter/duplicate/clear/ordinary/region-override/draw-order, real-cache load/shutdown");return 0;
}
