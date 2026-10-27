#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static socket_t peer;
static Isaac sender_cipher;
static void game_frame(const uint8_t *payload,size_t length,bool split) {
    uint8_t frame[257];frame[0]=(uint8_t)(4+isaac_next(&sender_cipher));frame[1]=(uint8_t)length;memcpy(frame+2,payload,length);
    if(split){assert(send(peer,(char*)frame,1,0)==1);assert(client_stream_wait_readable(&session.stream,1000)==1);int count=chatbox.message_count;assert(client_update() && chatbox.message_count==count);}
    size_t offset=split?1:0;assert(send(peer,(char*)frame+offset,(int)(length+2-offset),0)==(int)(length+2-offset));assert(client_stream_wait_readable(&session.stream,1000)==1);assert(client_update());assert(ingame && live_server && !reconnecting && !session.protocol_error);assert(!memcmp(&session.in_cipher,&sender_cipher,sizeof(sender_cipher)));
}
int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f && read_int(f)==0x47523235 && read_int(f)==2073);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);uint32_t seed[]={225,4,21,100};isaac_init(&session.in_cipher,seed,4);isaac_init(&sender_cipher,seed,4);session.in_game=true;live_server=true;ingame=true;chatbox_free(&chatbox);chatbox_init(&chatbox,"Self");int additions=0,trades=0,duels=0;
    for(int n=0;n<2073;n++){
        GameRequestInput in=game_request_input(f,&chatbox);private_override_chat225=in.override_chat!=0;sticky_chat_interface_id=in.sticky;interfaces.chat=in.dialog;strcpy(modal_message225,"modal kept");char *previous=chatbox.text[0];game_frame(in.payload,in.length,n%101==0);bool added=previous!=chatbox.text[0];GameRequestExpected expected=game_request_expected(f,&chatbox,added,n);assert(!strcmp(expected.modal,modal_message225));if(added){additions++;if(chatbox.type[0]==4)trades++;if(chatbox.type[0]==8)duels++;}
    }
    assert(fgetc(f)==EOF && additions==1385 && trades==294 && duels==286);fclose(f);
    /* Same stored draw-phase region override as private/public chat. */
    chatbox.ignore_count=0;sticky_chat_interface_id=-1;interfaces.chat=-1;private_override_chat225=false;streamer.base_tile_x=3100;streamer.base_tile_z=3100;player.base.x=player.base.z=0;assert(client_override_chat225());
    uint8_t request[]="Alice:tradereq:\n";game_frame(request,sizeof(request)-1,false);assert(chatbox.type[0]==4);streamer.scene_state=2;assert(client_draw() && private_override_chat225);char *previous=chatbox.text[0];game_frame(request,sizeof(request)-1,false);assert(chatbox.text[0]==previous);
    uint8_t malformed[]={'B','a','d'},recovery[]="Recovered\n";game_frame(malformed,sizeof(malformed),false);assert(chatbox.text[0]==previous);game_frame(recovery,sizeof(recovery)-1,false);assert(chatbox.type[0]==0 && !strcmp(chatbox.text[0],"Recovered"));
    closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();puts("PASS candidate game requests:2073 ISAAC-framed packet4 states,1385 additions,294 trades,286 duels;all100 history slots/full cipher/modal state;21 fragmented headers;draw-phase region override,malformed rejection/recovery;real cache,no production connection");return 0;
}
