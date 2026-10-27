#ifndef MUSIC_STATE_REFERENCE_CONTROL_H
#define MUSIC_STATE_REFERENCE_CONTROL_H
#include "core/music225.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wave_setting_reference_control.h"
static int music_record225;
static void music_check225(int ok,const char *label){if(!ok){fprintf(stderr,"music state record%d: %s\n",music_record225,label);exit(3);}}
static int music_byte225(FILE *f){int b=fgetc(f);music_check225(b!=EOF,"fixture byte");return b;}
static uint32_t music_int225(FILE *f){uint32_t a=(uint32_t)music_byte225(f)<<24;a|=(uint32_t)music_byte225(f)<<16;a|=(uint32_t)music_byte225(f)<<8;return a|(uint32_t)music_byte225(f);}
static bool music_string225(FILE *f,char out[64]){int n=(int32_t)music_int225(f);music_check225(n>=-1 && n<64,"string bound");memset(out,0,64);for(int i=0;i<n;i++)out[i]=(char)music_byte225(f);return n>=0;}
static Music225 music_state225(FILE *f){Music225 m;memset(&m,0,sizeof(m));m.active=music_byte225(f)!=0;m.low_memory=music_byte225(f)!=0;m.has_current=music_string225(f,m.current);m.crc=music_int225(f);m.size=music_int225(f);m.delay=(int32_t)music_int225(f);m.volume=(int32_t)music_int225(f);m.pending=music_string225(f,m.requested);m.requested_crc=music_int225(f);m.requested_size=music_int225(f);m.command=(int32_t)music_int225(f);m.fade=(int32_t)music_int225(f);return m;}
static void music_same225(const Music225 *m,const Music225 *e){
 music_check225(m->active==e->active && m->low_memory==e->low_memory,"enabled/low-memory");
 music_check225(m->has_current==e->has_current && (!m->has_current || !strcmp(m->current,e->current)) && m->crc==e->crc && m->size==e->size,"retained current song/metadata");
 music_check225(m->delay==e->delay && m->volume==e->volume && m->command==e->command && m->fade==e->fade,"delay/retained volume/command/fade");
 music_check225(m->pending==e->pending && (!m->pending || !strcmp(m->requested,e->requested)) && m->requested_crc==e->requested_crc && m->requested_size==e->requested_size,"latest request mailbox");
}
static void music_reference_run225(const char *path,Music225 *target,void (*execute)(int,int,const char *)){
 FILE *f=fopen(path,"rb");music_check225(f!=NULL,"open source fixture");music_check225(music_int225(f)==0x4d553236,"fixture magic");int count=(int)music_int225(f);music_check225(count==1800,"case count");music_check225(music_int225(f)==168,"source music varp");
 for(music_record225=0;music_record225<count;music_record225++){
  *target=music_state225(f);int op=(int)music_int225(f),value=(int32_t)music_int225(f);char song[64];music_check225(music_string225(f,song),"input song");Music225 expected=music_state225(f),after_tick=music_state225(f);
  if(execute){execute(op,value,song);music_same225(target,(op==1 || op==3)?&after_tick:&expected);}
  else {
   if(op==0)music_setting225(target,value);else if(op==1)music_check225(music_song225(target,song,0x87654321u,12345),"song accepted");else if(op==2)music_tick225(target);else if(op==3)(void)music_jingle225(target,value);else music_check225(0,"operation");
   music_same225(target,&expected);music_tick225(target);music_same225(target,&after_tick);
  }
 }
 music_check225(fgetc(f)==EOF,"fixture EOF");fclose(f);puts("PASS music state:1800 original settings/song54/real-cache jingle212/delay-slice states;full enabled/low-memory/retained metadata/pending mailbox/volume/fade/command;not worker/device acceptance");
}
static void music_setting_reference_run225(const char *path,VarpContext *v,ComponentConfig *cfg,GameSession *session){
 FILE *f=fopen(path,"rb");music_check225(f && music_int225(f)==0x4d533235 && music_int225(f)==960,"setting fixture");int id=(int)music_int225(f);music_check225(varp_type_clientcode(v->types,id)==3,"source cached music clientcode");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=music_int225(f);Arena arena=arena_create(1024);
 for(music_record225=0;music_record225<960;music_record225++){
  int op=(int)music_int225(f),prior=(int32_t)music_int225(f);bool active=music_byte225(f)!=0;int volume=(int32_t)music_int225(f),value=(int32_t)music_int225(f);memset(v->varps,0,sizeof(v->varps));memset(v->server_varps,0,sizeof(v->server_varps));v->varps[id]=prior;v->server_varps[id]=77;music_init225(&v->music);v->music.active=active;v->music.volume=volume;v->music.fade=7;v->music.has_current=true;strcpy(v->music.current,"adventure");v->music.crc=0x12345678;v->music.size=45;v->music.delay=21;v->music.pending=true;strcpy(v->music.requested,"old");v->music.requested_crc=33;v->music.requested_size=44;session->in_game=true;session->stream.out_len=0;session->stream.out_overflow=false;session->stream.buffered=true;session->protocol_error=false;isaac_init(&session->out_cipher,seed,4);
  ws_setting(op,id,value,v,cfg,session,&arena);music_check225((int32_t)music_int225(f)==v->varps[id] && (int32_t)music_int225(f)==v->server_varps[id],"local/server varps");Music225 expected=music_state225(f);music_same225(&v->music,&expected);int n=(int)music_int225(f);uint8_t wire[16];music_check225(n>=0 && n<16 && fread(wire,1,(size_t)n,f)==(size_t)n,"setting wire");music_check225(session->stream.out_len==(size_t)n && !memcmp(wire,session->stream.out_buf,(size_t)n),"setting full wire");ws_cipher(f,&session->out_cipher);
 }
 music_check225(fgetc(f)==EOF,"setting EOF");fclose(f);arena_free(&arena);session->stream.out_len=0;puts("PASS music setting paths:960 original direct/150/175/193/toggle/set states;full logical music/local/server/wire/ISAAC atpacket/action observer");
}
static void music_reference225(void){
 Music225 state;const char *path=getenv("RS2_MUSIC_STATE_ORACLE");music_reference_run225(path?path:"tests/fixtures/music-state-reference.bin",&state,NULL);
 Arena arena=arena_create(8*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);VarpTypeConfig types={0};music_check225(config && varp_type_unpack(&types,config),"music cached types");VarpContext v={0};v.types=&types;Component com={0};ComponentConfig cfg={.count=1,.instances=&com};GameSession *session=calloc(1,sizeof(*session));music_check225(session!=NULL,"music session");path=getenv("RS2_MUSIC_SETTING_ORACLE");music_setting_reference_run225(path?path:"tests/fixtures/music-setting-reference.bin",&v,&cfg,session);free(session);varp_type_free(&types);arena_free(&arena);
}
#endif
