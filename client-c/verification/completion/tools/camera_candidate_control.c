#include "dash3d/camera.h"
static Camera observed_camera;
static bool observed_camera_packet(Camera *c,int op,const uint8_t *p,size_t n,int level,int hm[4][105][105],uint8_t flags[4][104][104]){bool ok=camera_packet225(c,op,p,n,level,hm,flags);if(ok)observed_camera=*c;return ok;}
#define camera_packet225 observed_camera_packet
#include "../../../src/client/client.c"
#undef camera_packet225
#include "camera_reference_control.h"
static socket_t peer;
static Isaac sender_cipher;
static int packets_seen,draws_seen;
static void candidate_camera_packet(Camera *c,int op,const uint8_t *data,size_t n,int level){
 sync_ctx.current_plane=level;streamer.scene_state=0;uint8_t frame[7];frame[0]=(uint8_t)(op+isaac_next(&sender_cipher));memcpy(frame+1,data,n);assert(send(peer,(char*)frame,(int)n+1,0)==(int)n+1);assert(client_stream_wait_readable(&session.stream,1000)==1);assert(client_update() && !session.protocol_error && !reconnecting);assert(!memcmp(&session.in_cipher,&sender_cipher,sizeof(sender_cipher)));*c=observed_camera;packets_seen++;
}
static void candidate_camera_tick(Camera *c,int level){sync_ctx.current_plane=level;streamer.scene_state=c->cutscene?2:0;assert(client_update());ca_orbit(c);}
static void candidate_camera_draw(Camera *c,const Camera *expected,int level,uint64_t seed,int draw){
 if(draw%18)return;
 sync_ctx.current_plane=level;streamer.scene_state=2;player.base.x=player.base.z=6400;ca_orbit(c);Camera before=*c;math_random_seed((int64_t)seed);assert(client_draw());assert(!memcmp(c,&before,sizeof(*c)));const Ground225 *view=&scene_runtime225_result(scene_runtime)->view;int x=expected->eye_x,z=expected->eye_z;if(x<0)x=0;if(x>13311)x=13311;if(z<0)z=0;if(z>13311)z=13311;assert(view->eye_x==x && view->eye_y==expected->eye_y && view->eye_z==z && view->pitch==expected->render_pitch && view->yaw==expected->render_yaw);draws_seen++;
}
int main(int argc,char **argv){
 setvbuf(stdout,NULL,_IONBF,0);camera_fault_controls();assert(argc==2);ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
 socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);uint32_t seed[]={225,3,13,74};isaac_init(&session.in_cipher,seed,4);isaac_init(&sender_cipher,seed,4);session.in_game=true;ingame=live_server=true;idle_timeout=0;offline=false;player.visible=false;
 /* World.build has already built the map before the camera-only fixture
  * replaces heights and bridge flags. Keep that topology, as Java does. */
 assert(world3d->map_location_count>0);
 scene_runtime=scene_runtime225_create(world,world3d,&loc_cfg,&seq_cfg,&idk_cfg,&obj_cfg,&spot_cfg);assert(scene_runtime);
 printf("Populated camera fixture: %zu cache locations; topology built before synthetic camera data.\n",world3d->map_location_count);
 CameraReferenceHooks hooks={candidate_camera_packet,candidate_camera_tick,candidate_camera_draw};camera_reference_run(argv[1],&cam,world3d->heightmap,world->tile_flags,&hooks);assert(packets_seen==825 && draws_seen==50);closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();puts("PASS candidate camera:825 ISAAC-framed packet-phase observations,1552 real client_update ticks,50 actual client_draw views/base restores;full cipher,real cache,isolated connection");return 0;
}
