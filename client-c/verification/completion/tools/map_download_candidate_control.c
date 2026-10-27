#include "../../../src/client/client.c"
#define map_download_reference unused_map_native_reference
#include "map_download_reference.h"
#undef map_download_reference
static socket_t md_peer;
static Isaac md_sender;
static int md_fragments;
static void md_read_wire(GameSession *s,int length){
    s->stream.out_len=0;
    while(s->stream.out_len<(size_t)length){int n=recv(md_peer,(char*)s->stream.out_buf+s->stream.out_len,length-(int)s->stream.out_len,0);md_check(n>0,"candidate outgoing read");s->stream.out_len+=(size_t)n;}
    fd_set ready;FD_ZERO(&ready);FD_SET(md_peer,&ready);struct timeval wait={0,0};md_check(select(0,&ready,NULL,NULL,&wait)==0,"unexpected candidate outgoing bytes");
}
static void md_pin_tick(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;player.base.x=player.base.z=6400;player.base.path_length=0;for(int i=0;i<10;i++)player.base.path_tile_x[i]=player.base.path_tile_z[i]=50;}
static void md_frame(int op,const uint8_t *data,int n,bool split){
    uint8_t frame[65538];int header=1;frame[0]=(uint8_t)(op+isaac_next(&md_sender));if(op==237 || op==132 || op==220 || op==184){frame[1]=(uint8_t)(n>>8);frame[2]=(uint8_t)n;header=3;}memcpy(frame+header,data,(size_t)n);md_pin_tick();
    if(split){md_check(send(md_peer,(char*)frame,1,0)==1 && client_stream_wait_readable(&session.stream,1000)==1,"split opcode");md_check(client_update() && !session.protocol_error,"split tick");md_fragments++;}
    int at=split?1:0;md_check(send(md_peer,(char*)frame+at,header+n-at,0)==header+n-at && client_stream_wait_readable(&session.stream,1000)==1,"candidate send");md_pin_tick();md_check(client_update() && ingame && live_server && !reconnecting && !session.protocol_error,"candidate receive/update");md_check(!memcmp(&session.in_cipher,&md_sender,sizeof(md_sender)),"full incoming cipher");
}
int main(int argc,char **argv){
    setvbuf(stdout,NULL,_IONBF,0);md_check(argc==2,"candidate fixture argument");(void)unused_map_native_reference;
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};md_check(client_init(&options,&platform) && client_stream_global_init(),"candidate cache init");
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);md_check(listener!=INVALID_SOCKET_VAL,"listener");struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);md_check(!bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"loopback listen");int size=sizeof(address);md_check(!getsockname(listener,(struct sockaddr*)&address,&size),"port");protocol_init_session(&session);md_check(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"isolated connect");md_peer=accept(listener,NULL,NULL);md_check(md_peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);DWORD timeout=3000;setsockopt(md_peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
    md_file=fopen(argv[1],"rb");md_check(md_file && md_u32()==0x4d443235,"candidate fixture");int count=(int)md_u32();uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=md_u32();uint32_t incoming[]={225,237,184,991};isaac_init(&session.in_cipher,incoming,4);isaac_init(&md_sender,incoming,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;player.visible=false;
    char root[400],dir[500];snprintf(root,sizeof(root),"verification/completion/runs/map-candidate-%d",_getpid());md_check(!_mkdir(root),"fresh candidate cache");int reset=0,packets=0,builds=0;bool writes=true;md_wire_reader=md_read_wire;
    for(md_case=0;md_case<count;md_case++){
        int op=(int32_t)md_u32(),n=(int)md_u32();md_check(n>=0 && n<=65535,"candidate payload size");uint8_t *payload=malloc(n?(size_t)n:1);md_check(payload && fread(payload,1,(size_t)n,md_file)==(size_t)n,"candidate payload");session.stream.out_len=0;bool built=false;
        if(op==-1){snprintf(dir,sizeof(dir),"%s/cache-%d",root,reset++);md_check(!_mkdir(dir),"candidate scenario directory");md_reset_cache(payload,(size_t)n,dir);md_check(!_putenv_s("RS2_MAP_CACHE_DIR",dir),"isolated cache selection");map_streamer_clear(&streamer);scene_build_pending=false;build_old_base_x=build_old_base_z=0;world_reset(world);world3d_reset(world3d);effects_clear(&effects);zone_sync_reset(&zones);cam.cutscene=true;writes=true;isaac_init(&session.out_cipher,seed,4);}
        else if(op==-2){writes=payload[0]!=0;char failed[600];snprintf(failed,sizeof(failed),"%s/absent/subdirectory",dir);md_check(!_putenv_s("RS2_MAP_CACHE_DIR",writes?dir:failed),"cache failure selection");}
        else {built=op==184 && streamer.scene_state==1;md_frame(op,payload,n,md_case%31==0 && !built);packets++;if(built)builds++;}
        free(payload);md_snapshot(&streamer,world,collision_maps,&session,cam.cutscene,built,dir);
    }
    md_check(fgetc(md_file)==EOF,"candidate fixture tail");fclose(md_file);
    MapStreamer before=streamer;bool pending=scene_build_pending;uint8_t invalid[8]={50,50,0,5,0,4,1,2};int guards=0;for(int op=132;op<=220;op+=88){for(int n=0;n<6;n++){session.stream.out_len=0;md_frame(op,invalid,n,false);md_check(!memcmp(&streamer,&before,sizeof(before)) && pending==scene_build_pending,"candidate malformed state retained");md_read_wire(&session,0);guards++;}}
    session.stream.out_len=0;md_frame(132,invalid,sizeof(invalid),false);md_check(!memcmp(&streamer,&before,sizeof(before)),"candidate bad range retained");md_read_wire(&session,0);guards++;
    World *saved_world=malloc(sizeof(*world));CollisionMap saved_collision[4];md_check(saved_world!=NULL,"candidate retained-world allocation");memcpy(saved_world,world,sizeof(*world));memcpy(saved_collision,collision_maps,sizeof(saved_collision));
    uint8_t corrupt[]={50,50,0,0,0,5,0,0,0,1,0},done[]={50,50};session.stream.out_len=0;md_frame(132,corrupt,sizeof(corrupt),false);md_read_wire(&session,0);md_frame(80,done,2,false);md_read_wire(&session,0);uint8_t player_info[]={0,127,240};md_frame(184,player_info,3,false);md_read_wire(&session,0);md_check(streamer.scene_state==1 && scene_build_pending && !memcmp(world,saved_world,sizeof(*world)) && !memcmp(collision_maps,saved_collision,sizeof(saved_collision)),"candidate corrupt rebuild retains world/collision/loading");free(saved_world);
    uint8_t recovery_region[]={1,151,1,151,50,50,0,0,0,1,0,0,0,2};md_frame(237,recovery_region,sizeof(recovery_region),false);md_read_wire(&session,8);md_check(streamer.region_count==1 && streamer.scene_state==0,"candidate recovery rebase");
    for(int type=0;type<2;type++){char path[100];snprintf(path,sizeof(path),"../engine/data/pack/client/maps/%c50_50",type?'l':'m');FILE *f=fopen(path,"rb");md_check(f!=NULL,"candidate recovery real map");fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);uint8_t *bytes=malloc((size_t)length);md_check(bytes && fread(bytes,1,(size_t)length,f)==(size_t)length,"candidate recovery bytes");fclose(f);for(int at=0;at<length;at+=991){int size=(int)length-at;if(size>991)size=991;uint8_t payload[997]={50,50,(uint8_t)(at>>8),(uint8_t)at,(uint8_t)(length>>8),(uint8_t)length};memcpy(payload+6,bytes+at,(size_t)size);session.stream.out_len=0;md_frame(type?220:132,payload,size+6,false);md_read_wire(&session,0);}free(bytes);md_frame(type?20:80,done,2,false);md_read_wire(&session,0);}
    md_frame(184,player_info,3,false);md_read_wire(&session,4);md_check(streamer.scene_state==2 && !scene_build_pending,"candidate valid rebuild recovery");
    closesocket(md_peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();_putenv_s("RS2_MAP_CACHE_DIR","");printf("PASS candidate map download: %d original states,%d ISAAC-framed packets,%d actual client_update rebuilds,%d fragmented headers;full buffers/cache/outgoing+incoming cipher/heights/flags/collision;%d actual malformed receive/recovery guards;corrupt rebuild retention and real-map recovery;isolated cache %s\n",count,packets,builds,md_fragments,guards,root);return 0;
}
