#ifndef WAVE_SETTING_REFERENCE_CONTROL_H
#define WAVE_SETTING_REFERENCE_CONTROL_H
#include "ui/client_actions.h"
#include "ui/component_packets.h"
#include "config/varp_type.h"
#include "audio/wave.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static int ws_record;
static void ws_check(bool ok,const char *label){if(!ok){fprintf(stderr,"wave setting record%d: %s\n",ws_record,label);exit(3);}}
static int ws_int(FILE *f){uint8_t b[4];ws_check(fread(b,1,4,f)==4,"fixture integer");return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static bool ws_bool(FILE *f){int n=fgetc(f);ws_check(n==0 || n==1,"fixture boolean");return n!=0;}
static void ws_eq(FILE *f,int actual,const char *name){int expected=ws_int(f);if(expected!=actual){fprintf(stderr,"wave setting record%d %s expected%d actual%d\n",ws_record,name,expected,actual);exit(3);}}
static void ws_cipher(FILE *f,Isaac *a){ws_eq(f,a->count,"cipher count");ws_eq(f,(int32_t)a->a,"cipher a");ws_eq(f,(int32_t)a->b,"cipher b");ws_eq(f,(int32_t)a->c,"cipher c");for(int i=0;i<256;i++)ws_eq(f,(int32_t)a->rsl[i],"cipher rsl");for(int i=0;i<256;i++)ws_eq(f,(int32_t)a->mem[i],"cipher mem");}
typedef struct {void (*setting)(int,int,int);void (*packet)(const uint8_t*);void (*wire)(GameSession*,int);} WaveSettingHooks;
static void ws_setting(int op,int id,int value,VarpContext *v,ComponentConfig *cfg,GameSession *s,Arena *arena){
 if(op==0){v->varps[id]=value;varp_update225(v,id);return;}
 if(op==4 || op==5){int script[]={5,id,0},*scripts[]={script},length=3;Component saved=cfg->instances[0];cfg->instances[0]=(Component){.id=0,.type=4,.button_type=op,.script_count=1,.scripts=scripts,.script_lengths=&length,.script_comparator_count=1,.script_operand=&value};ClientActions a={.components=cfg,.varps=v,.session=s,.live=true};ws_check(client_action_dispatch(&a,(MenuAction){.type=MENU_IF_BUTTON,.component=0,.operation=op})==ACTION_SENT,"setting dispatch");cfg->instances[0]=saved;return;}
 uint8_t bytes[6]={(uint8_t)(id>>8),(uint8_t)id};size_t n=op==193?0:op==150?3:6;if(op==193)v->server_varps[id]=value;else if(op==150)bytes[2]=(uint8_t)value;else for(int i=0;i<4;i++)bytes[i+2]=(uint8_t)((uint32_t)value>>(24-8*i));ws_check(component_apply_packet(cfg,v,arena,op,bytes,n)==1,"varp packet");
}
static void wave_setting_reference_run(const char *path,VarpContext *v,ComponentConfig *cfg,GameSession *s,const WaveSettingHooks *hooks){
 FILE *f=fopen(path,"rb");ws_check(f && ws_int(f)==0x57533235,"source fixture");int count=ws_int(f),queues=ws_int(f),id=ws_int(f),trim=ws_int(f);ws_check(count==960 && queues==72 && varp_type_clientcode(v->types,id)==4,"cache metadata/counts");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)ws_int(f);Arena arena=arena_create(1024);
 for(ws_record=0;ws_record<count;ws_record++){int op=ws_int(f),prior=ws_int(f);v->wave_enabled=ws_bool(f);v->wave_volume=ws_int(f);audio_set_wave_volume225(v->wave_volume);int value=ws_int(f);memset(v->varps,0,sizeof(v->varps));memset(v->server_varps,0,sizeof(v->server_varps));v->varps[id]=prior;v->server_varps[id]=77;s->in_game=true;s->stream.out_len=0;s->stream.out_overflow=false;isaac_init(&s->out_cipher,seed,4);
  if(hooks)hooks->setting(op,id,value);else ws_setting(op,id,value,v,cfg,s,&arena);
  ws_eq(f,v->varps[id],"local");ws_eq(f,v->server_varps[id],"server");ws_check(v->wave_enabled==ws_bool(f),"enabled");ws_eq(f,v->wave_volume,"retained attenuation");int n=ws_int(f);uint8_t bytes[16];ws_check(n>=0 && n<16 && fread(bytes,1,(size_t)n,f)==(size_t)n,"wire bytes");if(hooks)hooks->wire(s,n);ws_check(s->stream.out_len==(size_t)n && !memcmp(bytes,s->stream.out_buf,(size_t)n),"full wire");ws_cipher(f,&s->out_cipher);
 }
 for(int row=0;row<queues;row++,ws_record++){audio_close();v->wave_enabled=ws_bool(f);int prior=ws_int(f),loops=ws_int(f),delay=ws_int(f);for(int i=0;i<prior;i++)audio_queue_sound(2,1,400+i-trim);uint8_t data[]={0,2,(uint8_t)loops,(uint8_t)(delay>>8),(uint8_t)delay};if(hooks)hooks->packet(data);else if(v->wave_enabled)audio_queue_sound(2,loops,delay);int expected=ws_int(f);ws_check(audio_queued_count()==expected,"queue admission/count");for(int i=0;i<expected;i++){AudioQueuedSound225 q;ws_check(audio_queue_get225(i,&q),"queue observation");ws_eq(f,q.sound_id,"queued id");ws_eq(f,q.loop_count,"raw loop byte");ws_eq(f,q.delay_ticks,"trimmed delay");}}
 ws_check(fgetc(f)==EOF,"fixture EOF");fclose(f);arena_free(&arena);audio_close();audio_set_wave_volume225(0);puts("PASS original wave setting:960 setting states/72 read12 admissions;full enabled/retained-volume/local/server/wire/ISAAC/queue;Off retains prior queue,raw loop0 andcapacity50");
}
static void wave_gain_reference(const char *path){FILE *f=fopen(path,"rb");ws_check(f && ws_int(f)==0x57473235 && ws_int(f)==4,"web gain fixture");for(int i=0;i<4;i++){int volume=ws_int(f);uint64_t bits=0;for(int j=0;j<8;j++){int n=fgetc(f);ws_check(n!=EOF,"gain bytes");bits=(bits<<8)|(unsigned)n;}double expected;memcpy(&expected,&bits,8);audio_set_wave_volume225(volume);ws_check(fabs(audio_wave_gain225()-expected)<1e-14,"unchanged web gain");for(int sample=0;sample<256;sample++){float converted=(float)((sample-128)*(expected/128.0));ws_check(audio_scale_sample225((uint8_t)sample)==converted,"float attenuation without extra source quantization");}}ws_check(fgetc(f)==EOF,"gain EOF");fclose(f);audio_set_wave_volume225(0);puts("PASS web wave gain:4 attenuation gains and1024 float output samples;no U8 requantization");}
static void wave_setting_registered(void){Arena arena=arena_create(8*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);VarpTypeConfig types={0};ws_check(config && varp_type_unpack(&types,config),"real metadata");VarpContext v={0};v.types=&types;Component com={0};ComponentConfig cfg={.count=1,.instances=&com};GameSession *s=calloc(1,sizeof(*s));ws_check(s!=NULL,"session allocation");s->stream.buffered=true;const char *path=getenv("RS2_WAVE_SETTING_ORACLE"),*gain=getenv("RS2_WAVE_GAIN_ORACLE");wave_setting_reference_run(path?path:"tests/fixtures/wave-setting-reference.bin",&v,&cfg,s,NULL);wave_gain_reference(gain?gain:"tests/fixtures/wave-gain-reference.bin");free(s);varp_type_free(&types);arena_free(&arena);}
#endif
