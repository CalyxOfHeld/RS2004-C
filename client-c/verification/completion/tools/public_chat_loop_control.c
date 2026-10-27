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
    if(opcode==21 || opcode==184)frame[n++]=(uint8_t)(length>>8);
    frame[n++]=(uint8_t)length;if(length)memcpy(frame+n,payload,length);n+=length;
    assert(send(peer,(char*)frame,(int)n,0)==(int)n);
    assert(client_stream_wait_readable(&session.stream,1000)==1);
    assert(client_update());assert(ingame && live_server && !reconnecting && !session.protocol_error);
}
static void control_bits(uint8_t *data,int *bit,int value,int count) {for(int b=count-1;b>=0;b--,(*bit)++)if((value>>b)&1)data[*bit/8]|=1<<(7-(*bit&7));}
static void public_control(int rights,int colour,const char *text,bool overflow,bool forced) {
    uint8_t bytes[512]={0},packed[100]={0};int bits=0;control_bits(bytes,&bits,1,1);control_bits(bytes,&bits,0,2);control_bits(bytes,&bits,0,8);control_bits(bytes,&bits,2047,11);
    Packet p=packet_wrap(bytes,sizeof(bytes));p.pos=(bits+7)/8;
    if(forced){p1(&p,8);for(size_t i=0;i<strlen(text);i++)p1(&p,(uint8_t)text[i]);p1(&p,10);}
    else {
        Packet raw=packet_wrap(packed,sizeof(packed));if(overflow)raw.pos=51;else assert(wordpack_pack(&raw,text,strlen(text),true));
        p1(&p,overflow?0xc0:0x40);if(overflow)p1(&p,1);p2(&p,colour);p1(&p,rights);p1(&p,(int)raw.pos);memcpy(bytes+p.pos,packed,raw.pos);p.pos+=raw.pos;
        if(overflow){p2(&p,65535);p4(&p,(19<<16)|23);}
    }
    packet_control(184,bytes,p.pos);
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
    strcpy(player.name,"Alice");strcpy(player.base.chat,"old");player.base.chat_timer=10;player.base.chat_color=13;player.base.chat_style=17;
    uint8_t ignored[8];Packet p=packet_wrap(ignored,8);p8(&p,(int64_t)jstring_to_base37("alice"));packet_control(21,ignored,8);
    public_control(0,0x1203,"ignored",false,false);assert(chatbox.message_count==0 && !strcmp(player.base.chat,"old") && player.base.chat_color==13 && player.base.chat_timer==8);
    public_control(2,0x1203,"fuck",false,false);assert(chatbox.message_count==1 && chatbox.type[0]==1 && !strcmp(player.base.chat,"****") && player.base.chat_color==18 && player.base.chat_style==3 && player.base.chat_timer==149);
    public_control(2,0x1203,"fuck",false,false);assert(chatbox.message_count==2); /* public has no private-message deduplication */
    packet_control(21,NULL,0);public_control(0,0xabcd,"Hello WORLD!",false,false);assert(chatbox.message_count==3 && chatbox.type[0]==2 && !strcmp(player.base.chat,"Hello world!") && player.base.chat_timer==149);
    public_control(2,0,"",true,false);assert(chatbox.message_count==3 && !strcmp(player.base.chat,"Hello world!") && player.base.spotanim_height==19 && player.base.chat_timer==148);
    private_override_chat225=true;
    public_control(2,0,"blocked",false,false);assert(chatbox.message_count==3 && player.base.chat_timer==147);
    public_control(0,0,"Raw fuck",false,true);assert(chatbox.message_count==4 && !strcmp(player.base.chat,"Raw fuck") && player.base.chat_color==0 && player.base.chat_style==0 && player.base.chat_timer==149);
    for(int i=0;i<149;i++)assert(client_update());
    assert(!player.base.chat[0] && player.base.chat_timer==0);
    closesocket(peer);protocol_close_session(&session);live_server=false;
    client_shutdown();client_stream_global_cleanup();
    puts("PASS candidate public dispatch:9 ISAAC-framed packets; ignore/staff/filter/repeated-public/clear/ordinary/overflow-suffix/override/forced, same-cycle timer decrement and149 subsequent expiry ticks");return 0;
}
