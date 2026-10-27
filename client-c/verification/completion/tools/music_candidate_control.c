#include "audio/midi.h"
static int mc_jingles;
static bool mc_observe=true;
static void mc_hold_apply(Music225 *music){if(!mc_observe)midi_apply_music225(music);}
static bool mc_hold_jingle(const uint8_t *data,int length,int decoded){mc_jingles++;return mc_observe?true:midi_request_jingle225(data,length,decoded);}
#define midi_apply_music225 mc_hold_apply
#define midi_request_jingle225 mc_hold_jingle
#define main wave_setting_control_unused_main
#include "wave_setting_candidate_control.c"
#undef main
#undef midi_apply_music225
#undef midi_request_jingle225
#include "music_state_reference_control.h"

static int mc_commands,mc_opens,mc_resets,mc_closes;
typedef struct {int attenuation;} MidiOutput225;
static bool mo_open225(MidiOutput225 **out,int attenuation){mc_commands++;mc_opens++;*out=calloc(1,sizeof(**out));if(!*out)return false;(*out)->attenuation=attenuation;return true;}
static bool mo_send225(MidiOutput225 *out,DWORD message,const uint8_t *bytes,size_t length){(void)out;(void)message;(void)bytes;(void)length;mc_commands++;return true;}
static bool mo_gain225(MidiOutput225 *out,int attenuation){out->attenuation=attenuation;mc_commands++;return true;}
static bool mo_reset225(MidiOutput225 *out){(void)out;mc_commands++;mc_resets++;return true;}
static bool mo_start225(MidiOutput225 *out){(void)out;mc_commands++;return true;}
static void mo_close225(MidiOutput225 *out){mc_commands++;mc_closes++;free(out);}
#define MIDI_DEVICE_SYNC_CONTROL 1
#define MIDI_OUTPUT225_TEST 1
#include "../../../src/audio/midi.c"
static uint8_t *mc_jingle;static size_t mc_jingle_length;static int mc_packets;
static void mc_tick(void){heartbeat_timer=cyclelogic1=cyclelogic2=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;session.idle_net_cycles=0;music_check225(client_update() && ingame && !reconnecting && !session.protocol_error,"candidate tick");}
static void mc_packet(int op,const uint8_t *data,size_t length){
 uint8_t frame[8192];size_t at=0;music_check225(length+3<sizeof(frame),"frame size");frame[at++]=(uint8_t)(op+isaac_next(&ws_sender));if(op==54)frame[at++]=(uint8_t)length;if(op==212){frame[at++]=(uint8_t)(length>>8);frame[at++]=(uint8_t)length;}memcpy(frame+at,data,length);at+=length;
 music_check225(send(ws_peer,(char*)frame,(int)at,0)==(int)at && client_stream_wait_readable(&session.stream,1000)==1,"deliver packet");mc_tick();music_check225(!memcmp(&session.in_cipher,&ws_sender,sizeof(ws_sender)),"full incoming cipher");mc_packets++;
}
static void mc_execute(int op,int value,const char *song){
 if(op==0){varp_ctx.varps[168]=value;varp_update225(&varp_ctx,168);}
 else if(op==1){uint8_t data[80];size_t n=strlen(song);memcpy(data,song,n);data[n++]=10;uint32_t values[]={0x87654321u,12345};for(int j=0;j<2;j++)for(int i=0;i<4;i++)data[n++]=(uint8_t)(values[j]>>(24-8*i));mc_packet(54,data,n);}
 else if(op==2)mc_tick();
 else if(op==3){uint8_t data[8192];data[0]=(uint8_t)(value>>8);data[1]=(uint8_t)value;memcpy(data+2,mc_jingle,mc_jingle_length);int before=mc_jingles;bool admit=varp_ctx.music.active && !varp_ctx.music.low_memory;mc_packet(212,data,mc_jingle_length+2);music_check225(mc_jingles==before+(admit?1:0),"jingle enabled/low-memory admission");}
}
static void mc_backend(void){
 midi_init();midi_set_silent(false);mc_commands=0;music_check225(midi_play("adventure",0,0),"real cache song/captured sequencer");
 Uint64 start=md_start225;int opens=mc_opens,resets=mc_resets,closes=mc_closes;
 midi_set_volume(128);music_check225(midi_is_playing() && !strcmp(midi_current_song(),"adventure") && md_start225==start && mc_opens==opens && mc_resets==resets && mc_closes==closes,"live volume retains timeline/device with no restart or reset");
 music_check225(midi_play_jingle(mc_jingle+4,(int)mc_jingle_length-4,(int)((uint32_t)mc_jingle[0]<<24 | (uint32_t)mc_jingle[1]<<16 | (uint32_t)mc_jingle[2]<<8 | mc_jingle[3]),0),"real cache jingle/captured sequencer");
 Music225 music;music_init225(&music);for(int level=0;level<4;level++){music_setting225(&music,level);midi_apply_music225(&music);int expected[]={0,-400,-800,-1200};music_check225(s_attenuation225==expected[level] && md_output225->attenuation==expected[level],"exact Java hundredths-dB output attenuation");}
 midi_set_silent(true);int before=mc_commands;midi_set_volume(180);music_check225(mc_commands==before,"silent replay never calls Windows output after device teardown");midi_set_silent(false);midi_close();puts("PASS music adapter:actual cache song/jingle decode andcaptured sequencer operations,4 gain policy levels,silent suppression;device scheduling qualified separately");
}
static uint64_t mc_long(FILE *f){uint64_t hi=music_int225(f);return hi<<32|music_int225(f);}
static uint64_t mc_hash(const uint8_t *p,int n){if(!p)return 0;uint64_t h=1469598103934665603ULL;for(int i=0;i<n;i++){h^=p[i];h*=1099511628211ULL;}return h;}
static void mc_slot(FILE *f,const uint8_t *bytes,int n){music_check225((int)music_int225(f)==n && mc_long(f)==mc_hash(bytes,n),"shared worker actual slot/publication bytes");}
static void mc_mixed(const char *path){
 audio_close();music_check225(audio_init(),"mixed captured device");audio_set_wave_volume225(0);ws_output_size=0;
 FILE *f=fopen(path,"rb");music_check225(f && music_int225(f)==0x4d423235 && music_int225(f)==68,"mixed fixture");static uint8_t wave_buffers[68][8192];int wave_published=-1;
 for(int row=0;row<68;row++){
  music_record225=row;int op=(int)music_int225(f),n=(int)music_int225(f),seed=(int)music_int225(f);bool fade=music_byte225(f)!=0;int result=-2;
  for(int i=0;i<8192;i++)wave_buffers[row][i]=(uint8_t)(i*17+seed);
  if(op==1)result=audio_request_save225(wave_buffers[row],n);
  else if(op==2)result=audio_request_replay225();
  else if(op==3){uint8_t *owned=malloc(8192);music_check225(owned!=NULL,"owned MIDI");memcpy(owned,wave_buffers[row],8192);result=audio_request_midi225(owned,n,fade)?1:0;if(!result)free(owned);}
  else if(op==4){if(s_wave_pending225==1 || s_wave_pending225==2)wave_published=s_wave_pos225;audio_service225();}
  music_check225((int32_t)music_int225(f)==result && (int)music_int225(f)==s_wave_pos225 && (int)music_int225(f)==s_midi_pos225,"shared request result/rotations");
  int pending=(int)music_int225(f),native_pending=s_wave_pending225==2?1:s_wave_pending225;
  music_check225(pending==native_pending && (int)music_int225(f)==s_wave_save_length225,"shared busy/retained length");
  music_check225((music_byte225(f)!=0)==(s_wave_pending225==1 || s_wave_pending225==2) && (music_byte225(f)!=0)==(s_wave_pending225==3) && (int)music_int225(f)==(s_midi_fade225?1:0),"shared flags/fade even rejected MIDI");
  for(int i=0;i<5;i++)mc_slot(f,s_wave_sizes225[i]?s_wave_slots225[i]:NULL,s_wave_sizes225[i]);
  for(int i=0;i<5;i++)mc_slot(f,s_midi_slots225[i],s_midi_sizes225[i]);
  mc_slot(f,wave_published<0?NULL:s_wave_slots225[wave_published],wave_published<0?0:s_wave_sizes225[wave_published]);
  mc_slot(f,s_midi_published225<0?NULL:s_midi_slots225[s_midi_published225],s_midi_published225<0?0:s_midi_sizes225[s_midi_published225]);
  if(wave_published>=0){int size=s_wave_sizes225[wave_published]-44;music_check225(ws_output_size==(size_t)size && !memcmp(ws_output_bytes,s_wave_slots225[wave_published]+44,(size_t)size),"actual mixed SDL publication");}
 }
 music_check225(fgetc(f)==EOF,"mixed EOF");fclose(f);audio_close();puts("PASS mixed audio:68 original actual-worker shared save states,both5-slot rotations,busy/drop/replay,10 slot buffers,fade onrejected MIDI andcaptured SDL publication");
 uint8_t *owned=malloc(128);music_check225(owned!=NULL,"worker MIDI buffer");memset(owned,9,128);music_check225(audio_request_midi225(owned,128,true) && audio_start_worker225(),"actual MIDI worker start");uint8_t *published=NULL;int length=0,slot=-1;bool fade=false,ambient=false;Uint64 start=SDL_GetTicks64();while(!audio_take_midi225(&published,&length,&fade,&ambient,&slot) && SDL_GetTicks64()-start<3000)SDL_Delay(1);music_check225(published && length==128 && fade && ambient && slot==1,"actual worker MIDI publication");for(int i=0;i<length;i++)music_check225(published[i]==9,"worker MIDI bytes");free(published);audio_close();music_check225(!s_wave_worker225 && !s_wave_mutex225 && !s_midi_pending225 && !s_midi_slots225[1],"worker joins/frees MIDI state");puts("PASS actual mixed MIDI worker:publication bytes/slot/role/fade andjoined cleanup");
}
static uint64_t mc_now;
static uint64_t mc_clock(void){mc_now+=20;return mc_now;}
static void mc_song(const char *name){
 char path[256];snprintf(path,sizeof(path),"../engine/data/pack/client/songs/%s.mid",name);FILE *f=fopen(path,"rb");music_check225(f!=NULL,"end-to-end song");fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);uint8_t *packed=malloc((size_t)size);music_check225(packed && fread(packed,1,(size_t)size,f)==(size_t)size,"song bytes");fclose(f);uint32_t values[]={midi_crc32(packed,(size_t)size),(uint32_t)size};free(packed);
 uint8_t data[96];size_t n=strlen(name);memcpy(data,name,n);data[n++]=10;for(int j=0;j<2;j++)for(int i=0;i<4;i++)data[n++]=(uint8_t)(values[j]>>(24-8*i));mc_packet(54,data,n);
}
static void mc_flow(void){
 mc_observe=false;ws_run_tick=true;audio_close();music_check225(audio_init(),"flow captured device");music_init225(&varp_ctx.music);varp_ctx.wave_enabled=true;varp_ctx.varps[168]=0;mc_now=1000000;plat.tracking_now_ms=mc_clock;midi_init();midi_set_silent(false);
 mc_song("adventure");music_check225(s_wave_pending225==3 && !varp_ctx.music.pending,"real song queues shared save before publication");
 uint8_t sound[]={0,2,1,0,0};mc_packet(12,sound,5);music_check225(audio_queued_count()==1 && s_sound_queue225.entries[0].delay_ticks==-5 && s_is_playing,"MIDI busy causes one sound retry;actual publication starts song");int before=ws_output_calls;mc_tick();music_check225(audio_queued_count()==0 && ws_output_calls==before+1,"sound retry submits alongside music");
 uint8_t off[]={0,168,4};mc_packet(150,off,3);music_check225(!varp_ctx.music.active && !s_is_playing && varp_ctx.music.has_current && !strcmp(varp_ctx.music.current,"adventure"),"Off stops device andretains logical song");
 mc_song("newbie_melody");music_check225(!s_wave_pending225 && !strcmp(varp_ctx.music.current,"newbie_melody"),"disabled song updates metadata without request");
 uint8_t on[]={0,168,0,0,0,1};mc_packet(175,on,6);music_check225(varp_ctx.music.active && varp_ctx.music.volume==-400 && s_attenuation225==-400 && s_wave_pending225==3,"On queues retained song withsource attenuation");
 uint8_t jingle[8192];jingle[0]=0;jingle[1]=40;memcpy(jingle+2,mc_jingle,mc_jingle_length);mc_packet(212,jingle,mc_jingle_length+2);music_check225(varp_ctx.music.delay==20 && !s_wave_pending225 && !s_midi_fade225 && s_is_playing,"busy jingle drops butretains source delay/fade;ambient role unchanged");mc_tick();music_check225(varp_ctx.music.delay==0 && s_wave_pending225==3,"same-cycle countdown reaches source ambient restart");mc_tick();music_check225(!s_wave_pending225 && s_is_playing,"restarted ambient publishes");
 varp_ctx.music.low_memory=true;mc_packet(12,sound,5);int jingles=mc_jingles;mc_packet(212,mc_jingle,0);music_check225(!audio_queued_count() && mc_jingles==jingles,"low-memory suppresses wave andjingle decode");varp_ctx.music.low_memory=false;
 varp_ctx.server_varps[168]=4;mc_packet(193,mc_jingle,0);music_check225(!varp_ctx.music.active && !s_is_playing,"193 Off reaches native stop");varp_ctx.server_varps[168]=0;mc_packet(193,mc_jingle,0);music_check225(varp_ctx.music.active && s_wave_pending225==3,"193 On queues retained source song");
 Music225 retained=varp_ctx.music;
 for(int code=1;code<=8;code++)if(code==1 || code==4 || code==5 || code==6 || code==8){
  int id=-1;for(int i=0;i<varp_ctx.types->count;i++)if(varp_type_clientcode(varp_ctx.types,i)==code){id=i;break;}music_check225(id>=0,"integrated cached setting code");int value=code==1 || code==4?2:1;uint8_t setting[]={ (uint8_t)(id>>8),(uint8_t)id,(uint8_t)value };mc_packet(150,setting,3);music_check225(!memcmp(&retained,&varp_ctx.music,sizeof(retained)) && s_attenuation225==0,"other settings retain music/volume");music_check225(varp_ctx.varps[id]==value,"integrated setting value");
 }
 music_check225(varp_ctx.mouse_buttons_option==1 && varp_ctx.wave_enabled && varp_ctx.wave_volume==-800 && fabs(audio_wave_gain225()-pow(10.0,-800.0/2000.0))<1e-14,"mouse andindependent sound gain while music plays");puts("PASS combined settings:all6 cached clientcodes1/3/4/5/6/8 inactual candidate,independent sound/music levels andretained music state");
 midi_close();audio_close();mc_observe=true;ws_run_tick=false;plat.tracking_now_ms=NULL;puts("PASS integrated audio:actual framed150/175/54/212/12,Off/retained-song On,shared MIDI busy/sound retry,source jingle delay/restart,volume andlow-memory suppression;Windows output/SDL captured");
}
static void mc_log_pass(void){
 uint8_t *a=malloc(128),*b=malloc(64);music_check225(a && b,"log buffers");for(int i=0;i<128;i++)a[i]=(uint8_t)(i*17+9);memset(b,7,64);
 music_check225(audio_request_midi225(a,128,false),"logged MIDI accepted");music_check225(!audio_request_midi225(b,64,true),"logged MIDI busy drop");free(b);music_check225(audio_request_replay225()==0,"logged wave busy outcome");audio_service225();uint8_t *published=NULL;int length=0,slot=-1;bool fade=false,ambient=true;music_check225(audio_take_midi225(&published,&length,&fade,&ambient,&slot) && length==128 && fade && !ambient && slot==1,"logged publication metadata");for(int i=0;i<128;i++)music_check225(published[i]==(uint8_t)(i*17+9),"logged full publication bytes");free(published);
}
static void mc_log(const char *path){
 audio_close();music_check225(session_log_record(path),"fresh music log");mc_log_pass();session_log_close();audio_close();music_check225(session_log_replay(path),"replay music log");mc_log_pass();music_check225(!session_log_failed() && session_log_at_end(),"music log end");session_log_close();audio_close();puts("PASS mixed audio log:accepted/dropped MQ hashes,outcome andfade,recorded AQ busy,MP complete bytes/role/fade;no live worker dependency onreplay");
}
int main(int argc,char **argv){
 if(argc==3 && !strcmp(argv[1],"--replay-log")){audio_close();music_check225(session_log_replay(argv[2]),"replay-only log");mc_log_pass();music_check225(!session_log_failed() && session_log_at_end(),"replay-only end");session_log_close();audio_close();puts("PASS replay-only mixed audio");return 0;}
 setvbuf(stdout,NULL,_IONBF,0);music_check225(argc==5,"fixture/log arguments");(void)music_reference225;
 ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};music_check225(client_init(&options,&platform) && client_stream_global_init(),"real cache init");
 FILE *f=fopen("../engine/data/pack/client/jingles/advance cooking.mid","rb");music_check225(f!=NULL,"jingle cache");fseek(f,0,SEEK_END);mc_jingle_length=(size_t)ftell(f);rewind(f);mc_jingle=malloc(mc_jingle_length);music_check225(mc_jingle && fread(mc_jingle,1,mc_jingle_length,f)==mc_jingle_length,"jingle bytes");fclose(f);
 socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);music_check225(listener!=INVALID_SOCKET_VAL && !bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"listen");int size=sizeof(address);music_check225(!getsockname(listener,(struct sockaddr*)&address,&size),"port");protocol_init_session(&session);music_check225(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"connect");ws_peer=accept(listener,NULL,NULL);music_check225(ws_peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);uint32_t seed[]={225,3,54,212};isaac_init(&session.in_cipher,seed,4);isaac_init(&ws_sender,seed,4);session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
 music_reference_run225(argv[1],&varp_ctx.music,mc_execute);printf("PASS candidate music:%d framed song/jingle packets,actual client updates andfull incoming cipher;device drain held atstate observer\n",mc_packets);music_setting_reference_run225(argv[3],&varp_ctx,&comp_cfg,&session);mc_backend();mc_mixed(argv[2]);mc_flow();mc_log(argv[4]);free(mc_jingle);closesocket(ws_peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();return 0;
}
