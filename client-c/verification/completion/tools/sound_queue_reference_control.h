#ifndef SOUND_QUEUE_REFERENCE_CONTROL_H
#define SOUND_QUEUE_REFERENCE_CONTROL_H
#include "core/sound_queue.h"
#include "audio/wave.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int kind,a,b;int64_t value;} SqEvent;
static SqEvent sq_events[256];static int sq_event_count,sq_record,sq_spec,sq_clocks;static bool sq_save_throws,sq_replay_throws;static int64_t sq_time,sq_step;static uint8_t sq_synthetic[441000];
static void sq_check(bool ok,const char *label){if(!ok){fprintf(stderr,"sound queue record%d: %s\n",sq_record,label);exit(3);}}
static int sq_int(FILE *f){uint8_t b[4];sq_check(fread(b,1,4,f)==4,"fixture integer");return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static int64_t sq_long(FILE *f){uint64_t bits=0;for(int i=0;i<8;i++){int b=fgetc(f);sq_check(b!=EOF,"fixture long");bits=bits<<8|(unsigned)b;}int64_t n;memcpy(&n,&bits,8);return n;}
static bool sq_bool(FILE *f){int n=fgetc(f);sq_check(n==0 || n==1,"fixture boolean");return n!=0;}
static void sq_eq(FILE *f,int actual,const char *label){int expected=sq_int(f);if(actual!=expected){fprintf(stderr,"sound queue record%d %s expected%d actual%d\n",sq_record,label,expected,actual);exit(3);}}
static uint64_t sq_hash(const uint8_t *bytes,int length){if(!bytes)return 0;uint64_t h=1469598103934665603ULL;for(int i=0;i<length;i++){h^=bytes[i];h*=1099511628211ULL;}return h;}
static void sq_event(int kind,int a,int b,int64_t value){sq_check(sq_event_count<256,"trace bound");sq_events[sq_event_count++]=(SqEvent){kind,a,b,value};}
static void sq_read_state(FILE *f,SoundQueue225 *q){q->count=sq_int(f);q->last_id=sq_int(f);q->last_loops=sq_int(f);q->last_length=sq_int(f);q->last_start=sq_long(f);for(int i=0;i<50;i++){q->entries[i].sound_id=sq_int(f);q->entries[i].loop_count=sq_int(f);q->entries[i].delay_ticks=sq_int(f);}}
static void sq_compare_state(FILE *f,const SoundQueue225 *q){sq_eq(f,q->count,"count");sq_eq(f,q->last_id,"last id");sq_eq(f,q->last_loops,"last loops");sq_eq(f,q->last_length,"last length");sq_check(q->last_start==sq_long(f),"last start");for(int i=0;i<50;i++){sq_eq(f,q->entries[i].sound_id,"slot id");sq_eq(f,q->entries[i].loop_count,"slot loops");sq_eq(f,q->entries[i].delay_ticks,"slot delay");}}
static int sq_generate(void *user,int id,int loops,const uint8_t **bytes){(void)user;int length=sq_spec;
 if(sq_spec==-3){size_t n=0;*bytes=wave_generate(id,loops,NULL,&n);length=*bytes?(int)n:-1;}
 else if(sq_spec<0){*bytes=NULL;length=-1;}
 else {for(int i=0;i<length;i++)sq_synthetic[i]=(uint8_t)(i*17+id*31+loops);*bytes=length?sq_synthetic:NULL;}
 sq_event(1,id,loops,length);if(length>=0)sq_event(5,length,0,(int64_t)sq_hash(*bytes,length));return length;
}
static int64_t sq_clock(void){uint64_t bits=(uint64_t)sq_time+(uint64_t)sq_step*(uint64_t)sq_clocks++;int64_t n;memcpy(&n,&bits,8);sq_event(2,0,0,n);return n;}
static int64_t sq_now(void *user){(void)user;return sq_clock();}
static int sq_save(void *user,const uint8_t *bytes,int length){(void)user;int result=sq_save_throws?-1:audio_request_save225(bytes,length);sq_event(3,length,result,(int64_t)sq_hash(bytes,length));return result;}
static int sq_replay(void *user){(void)user;int result=sq_replay_throws?-1:audio_request_replay225();sq_event(4,result,0,0);return result;}
static void sound_queue_reference_run(const char *path,SoundQueue225 *q,void (*tick)(void)){
 FILE *f=fopen(path,"rb");sq_check(f && sq_int(f)==0x53513235,"original fixture");int count=sq_int(f),real=sq_int(f);sq_check(count==2326 && real==1740,"source counts");
 for(sq_record=0;sq_record<count;sq_record++){
  audio_close();sq_spec=sq_int(f);sq_time=sq_long(f);sq_step=sq_long(f);bool busy=sq_bool(f);sq_save_throws=sq_bool(f);sq_replay_throws=sq_bool(f);sq_read_state(f,q);sq_clocks=sq_event_count=0;
  if(busy)sq_check(audio_request_replay225()==1,"initial busy boundary");
  if(tick)tick();else {SoundQueueOps225 ops={NULL,sq_generate,sq_now,sq_save,sq_replay};sq_check(sound_queue_tick225(q,&ops),"native tick");}
  sq_compare_state(f,q);sq_eq(f,sq_clocks,"source clock calls");sq_eq(f,sq_event_count,"source event count");for(int i=0;i<sq_event_count;i++){sq_eq(f,sq_events[i].kind,"trace kind");sq_eq(f,sq_events[i].a,"trace a");sq_eq(f,sq_events[i].b,"trace b");sq_check(sq_events[i].value==sq_long(f),"trace time/hash/result");}
 }
 sq_check(fgetc(f)==EOF,"fixture EOF");fclose(f);audio_close();printf("PASS original sound queue:2326 full50-slot states andordered generation/time/save/replay traces;1740 actual cache generations/loops0,1,2,255;strict overlap,signed-time wrap,retry-5/drop,exceptions,stale tail andclock-call counts\n");
}
static void sound_queue_guard_controls(void){
 SoundQueue225 q,before;sound_queue_init225(&q);SoundQueueOps225 ops={NULL,sq_generate,sq_now,sq_save,sq_replay};before=q;sq_clocks=sq_event_count=0;
 sq_check(!sound_queue_tick225(NULL,&ops) && !sound_queue_tick225(&q,NULL) && !memcmp(&q,&before,sizeof(q)),"null state/ops guard");
 q.count=-1;before=q;sq_check(!sound_queue_tick225(&q,&ops) && !sound_queue_add225(&q,2,1,0) && !memcmp(&q,&before,sizeof(q)),"negative count guard");q.count=51;before=q;sq_check(!sound_queue_tick225(&q,&ops) && !memcmp(&q,&before,sizeof(q)),"over-capacity guard");
 q.count=50;before=q;sq_check(!sound_queue_add225(&q,2,1,0) && !memcmp(&q,&before,sizeof(q)),"full enqueue retains state");q.count=0;before=q;sq_check(sound_queue_tick225(&q,&ops) && !memcmp(&q,&before,sizeof(q)) && !sq_clocks && !sq_event_count,"empty queue has no callbacks");puts("PASS sound queue guards:null/invalid counts/full enqueue/empty callback suppression");
}
static void sound_queue_registered(void){SoundQueue225 q;const char *path=getenv("RS2_SOUND_QUEUE_ORACLE");sound_queue_reference_run(path?path:"tests/fixtures/sound-queue-reference.bin",&q,NULL);sound_queue_guard_controls();}
#endif
