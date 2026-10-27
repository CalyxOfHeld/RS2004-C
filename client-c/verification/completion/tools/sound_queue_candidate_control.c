#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "net/protocol.h"
#include "core/sound_queue.h"
#include <string.h>
static bool sq_controlled,sq_inject_login;
static LoginResponse sq_login_reply;
static LoginResponse sq_login(GameSession *s,const char *host,int port,const char *name,const char *password,bool low,bool reconnect){(void)host;(void)port;(void)name;(void)password;(void)low;(void)reconnect;if(!sq_inject_login)abort();s->in_game=true;s->is_mod=sq_login_reply==LOGIN_MOD_SUCCESS;return sq_login_reply;}
#define protocol_login_ex sq_login
#include "../../../src/client/client.c"
#undef protocol_login_ex
#include "client/client_log.h"
static int sq_generate(void*,int,int,const uint8_t**);
static int sq_save(void*,const uint8_t*,int);
static int sq_replay(void*);
static bool sq_tick_observer(SoundQueue225 *q,const SoundQueueOps225 *ops){if(!sq_controlled)return sound_queue_tick225(q,ops);SoundQueueOps225 copy=*ops;copy.generate=sq_generate;copy.save=sq_save;copy.replay=sq_replay;return sound_queue_tick225(q,&copy);}
static uint8_t sq_output[441000];static size_t sq_output_size;
static SDL_atomic_t sq_output_calls;
static int SDLCALL sq_capture(SDL_AudioDeviceID id,const void *data,Uint32 n){(void)id;if(n>sizeof(sq_output))return -1;memcpy(sq_output,data,n);sq_output_size=n;SDL_AtomicAdd(&sq_output_calls,1);return 0;}
static SDL_AudioDeviceID SDLCALL sq_open(const char *name,int capture,const SDL_AudioSpec *want,SDL_AudioSpec *have,int flags){(void)name;(void)capture;(void)flags;*have=*want;return 7;}
static void SDLCALL sq_pause(SDL_AudioDeviceID id,int pause){(void)id;(void)pause;}
static void SDLCALL sq_close(SDL_AudioDeviceID id){(void)id;}
#define sound_queue_tick225 sq_tick_observer
#define SDL_QueueAudio sq_capture
#define SDL_OpenAudioDevice sq_open
#define SDL_PauseAudioDevice sq_pause
#define SDL_CloseAudioDevice sq_close
#include "../../../src/audio/wave.c"
#undef sound_queue_tick225
#undef SDL_QueueAudio
#undef SDL_OpenAudioDevice
#undef SDL_PauseAudioDevice
#undef SDL_CloseAudioDevice
#include "sound_queue_reference_control.h"
static socket_t sq_peer;static Isaac sq_sender;static int sq_packets;
static uint64_t sq_platform_clock(void){return (uint64_t)sq_clock();}
static void sq_pin(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;}
static void sq_native_tick(void){sq_pin();sq_check(client_update() && ingame && !reconnecting,"actual candidate tick");}
static void sq_packet(int op,const uint8_t *data,size_t n){uint8_t frame[8];frame[0]=(uint8_t)(op+isaac_next(&sq_sender));if(n)memcpy(frame+1,data,n);sq_check(send(sq_peer,(char*)frame,(int)n+1,0)==(int)n+1 && client_stream_wait_readable(&session.stream,1000)==1,"candidate framed input");sq_pin();sq_check(client_update() && (op==142?!ingame:ingame && !reconnecting && !session.protocol_error),"candidate packet/logout update");sq_check(!memcmp(&sq_sender,&session.in_cipher,sizeof(sq_sender)),"incoming cipher");sq_packets++;}
static void sq_backend(const char *path){
 audio_close();sq_check(audio_init(),"captured device");audio_set_wave_volume225(0);static uint8_t buffers[2][441000];sq_output_size=0;SDL_AtomicSet(&sq_output_calls,0);FILE *f=fopen(path,"rb");sq_check(f && sq_int(f)==0x53423235 && sq_int(f)==67,"original backend fixture");
 for(int row=0;row<67;row++){sq_record=row;int op=sq_int(f),a=sq_int(f),b=sq_int(f),seed=sq_int(f),result=-2;
  if(op==1){for(int i=0;i<441000;i++)buffers[a][i]=(uint8_t)(i*17+seed);result=audio_request_save225(buffers[a],b);}
  else if(op==2)result=audio_request_replay225();else if(op==3)buffers[a][b]^=(uint8_t)seed;else if(op==4)audio_service225();else if(op==5)result=audio_request_save225(NULL,b);
  sq_eq(f,result,"backend request result");sq_eq(f,s_wave_pos225,"slot rotation");sq_check((s_wave_pending225!=0)==sq_bool(f),"backend busy");sq_check((s_wave_save225!=NULL)==sq_bool(f),"saved buffer presence");sq_eq(f,s_wave_save_length225,"retained save length");sq_check((s_wave_pending225!=0)==sq_bool(f),"worker waveplay");
  for(int i=0;i<5;i++){sq_eq(f,s_wave_sizes225[i],"slot length");sq_check((int64_t)(s_wave_sizes225[i]?sq_hash(s_wave_slots225[i],s_wave_sizes225[i]):0)==sq_long(f),"actual worker slot bytes");}
  sq_check((int64_t)(s_wave_save225?sq_hash(s_wave_save225,s_wave_save_length225):0)==sq_long(f),"pending/retained alias");sq_eq(f,(int)sq_output_size,"published PCM size");sq_check((int64_t)(sq_output_size?sq_hash(sq_output,(int)sq_output_size):0)==sq_long(f),"published PCM bytes");
 }
 sq_check(fgetc(f)==EOF,"backend EOF");fclose(f);audio_close();
 sq_check(audio_init(),"worker device");memset(buffers[0],129,64);sq_check(audio_request_save225(buffers[0],64)==1 && audio_request_replay225()==0,"pending before worker");int before=SDL_AtomicGet(&sq_output_calls);sq_check(audio_start_worker225() && audio_start_worker225(),"one bounded worker");Uint64 start=SDL_GetTicks64();while(SDL_AtomicGet(&sq_output_calls)==before && SDL_GetTicks64()-start<3000)SDL_Delay(1);sq_check(SDL_AtomicGet(&sq_output_calls)>before,"worker completion");audio_close();sq_check(!s_wave_worker225 && !s_wave_mutex225,"worker joined andclosed");puts("PASS wave backend:67 original actual-worker states,5 slots/busy/replay/null/oversize/alias bytes;actual SDL worker completes andjoins;device calls captured");
}
static int sq_logged_live_calls;static int64_t sq_logged_time;
static uint64_t sq_live_clock(void){sq_logged_live_calls++;return (uint64_t)sq_logged_time++;}
static uint64_t sq_logged_clock(void){return client_log_time(sq_live_clock);}
static void sq_seed_log(void){audio_close();audio_queue_cold225();audio_queue_sound(2,1,0);audio_queue_sound(2,1,0);}
static void sq_log(const char *path,const char *states,bool replay_only){
 sq_controlled=false;plat.tracking_now_ms=sq_logged_clock;SoundQueue225 expected[3];
 if(!replay_only){FILE *old=fopen(path,"rb");sq_check(!old,"fresh log");sq_seed_log();sq_logged_time=1000000;sq_logged_live_calls=0;sq_check(session_log_record(path),"record audio fragment");for(int i=0;i<3;i++){sq_native_tick();expected[i]=s_sound_queue225;}session_log_close();sq_check(sq_logged_live_calls==2,"source exact two overlap clock reads");FILE *f=fopen(states,"wb");sq_check(f && fwrite(expected,sizeof(expected),1,f)==1,"record per-tick states");fclose(f);}
 FILE *f=fopen(states,"rb");sq_check(f && fread(expected,sizeof(expected),1,f)==1 && fgetc(f)==EOF,"saved tick states");fclose(f);sq_seed_log();sq_logged_time=-99999;sq_logged_live_calls=0;sq_check(session_log_replay(path),"replay audio fragment");for(int i=0;i<3;i++){sq_native_tick();sq_check(!memcmp(&expected[i],&s_sound_queue225,sizeof(SoundQueue225)) && !session_log_failed(),"per-tick logged audio state");}sq_check(session_log_at_end() && !sq_logged_live_calls,"no live clock/output dependency on replay");session_log_close();audio_close();plat.tracking_now_ms=sq_platform_clock;puts("PASS logged sound lifecycle:3 actual candidate ticks,2 recorded wall-clock reads,3 request outcomes/full generated-WAV hashes,all50 slots each tick;wrong live clock unused");
}
static void sq_integration(void){
 sq_controlled=false;plat.tracking_now_ms=sq_platform_clock;sq_time=1000000;sq_step=1;sq_clocks=0;audio_close();varp_ctx.wave_enabled=true;uint8_t sound[]={0,2,1,0,1};sq_packet(12,sound,5);sq_check(audio_queued_count()==1 && s_sound_queue225.entries[0].delay_ticks==0,"read12 then same-cycle sound decrement");sq_packet(12,sound,5);sq_check(audio_queued_count()==1 && s_sound_queue225.entries[0].delay_ticks==0,"due sound consumed before next queued sound");
 uint8_t off[]={0,169,4};sq_packet(150,off,3);sq_check(!varp_ctx.wave_enabled && !audio_queued_count(),"Off doesnot cancel due queued replay");sq_packet(12,sound,5);sq_check(!audio_queued_count(),"Off admission suppression");
 for(int reply=LOGIN_SUCCESS;reply<=LOGIN_MOD_SUCCESS;reply+=16){SoundQueue225 before=s_sound_queue225;before.count=1;before.entries[0]=(AudioQueuedSound225){2,1,9};s_sound_queue225=before;sq_inject_login=true;sq_login_reply=(LoginResponse)reply;puts("[SOUND CONTROL] Injected native login reply;no authentication/account");client_login(false);sq_inject_login=false;before.count=0;sq_check(!memcmp(&before,&s_sound_queue225,sizeof(before)),"normal login clears count only");}
 s_sound_queue225.count=1;SoundQueue225 before=s_sound_queue225;sq_inject_login=true;sq_login_reply=LOGIN_RECONNECT_SUCCESS;client_login(true);sq_inject_login=false;sq_check(!memcmp(&before,&s_sound_queue225,sizeof(before)),"reconnect retains sound state");sq_packet(142,NULL,0);sq_check(!ingame && !memcmp(&before,&s_sound_queue225,sizeof(before)),"logout retains unticked queue until next normal login");uint8_t byte;sq_check(recv(sq_peer,(char*)&byte,1,0)==0,"server logout closes peer");audio_close();puts("PASS candidate sound integration:framed12 same-tick delays/settings150 Off,actual native injected2/18 count-only reset and15 retention,framed142 logout retention");
}
int main(int argc,char **argv){setvbuf(stdout,NULL,_IONBF,0);bool replay_only=argc==4 && !strcmp(argv[1],"--replay-log"),backend_only=argc==3 && !strcmp(argv[1],"--backend-only");sq_check(replay_only || backend_only || argc==5,"fixture/backend/log/state arguments");(void)sound_queue_registered;ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};platform.tracking_now_ms=sq_platform_clock;sq_check(client_init(&options,&platform) && client_stream_global_init(),"real-cache init");uint32_t noise=0x12345678;for(int i=0;i<32768;i++){noise=noise*1103515245+12345;s_noise[i]=(noise&0x80000000)?1:-1;}
 socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);sq_check(listener!=INVALID_SOCKET_VAL && !bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"isolated listen");int size=sizeof(address);sq_check(!getsockname(listener,(struct sockaddr*)&address,&size),"port");protocol_init_session(&session);sq_check(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"connect");sq_peer=accept(listener,NULL,NULL);sq_check(sq_peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);DWORD timeout=3000;setsockopt(sq_peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));uint32_t seed[]={225,12,50,5};isaac_init(&session.in_cipher,seed,4);isaac_init(&sq_sender,seed,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
 if(backend_only)sq_backend(argv[2]);else if(replay_only)sq_log(argv[2],argv[3],true);else {sq_controlled=true;sound_queue_reference_run(argv[1],&s_sound_queue225,sq_native_tick);sq_controlled=false;sq_backend(argv[2]);sq_log(argv[3],argv[4],false);sq_integration();}
 closesocket(sq_peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();puts("PASS actual candidate sound queue:source core/worker/clock outcomes andnormal lifecycle,isolated loopback andcaptured device");return 0;}
