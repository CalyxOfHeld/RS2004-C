#include "core/sound_queue.h"
#include <string.h>
void sound_queue_init225(SoundQueue225 *q){if(q){memset(q,0,sizeof(*q));q->last_id=q->last_loops=-1;}}
bool sound_queue_add225(SoundQueue225 *q,int id,int loops,int delay){if(!q || q->count<0 || q->count>=50)return false;q->entries[q->count++]=(AudioQueuedSound225){id,loops,delay};return true;}
static int64_t add225(int64_t a,int b){uint64_t bits=(uint64_t)a+(uint64_t)(int64_t)b;int64_t value;memcpy(&value,&bits,sizeof(value));return value;}
bool sound_queue_tick225(SoundQueue225 *q,const SoundQueueOps225 *ops){
 if(!q || q->count<0 || q->count>50 || !ops || !ops->generate || !ops->now || !ops->save || !ops->replay)return false;
 for(int i=0;i<q->count;i++){
  AudioQueuedSound225 *e=&q->entries[i];
  if(e->delay_ticks>0){e->delay_ticks--;continue;}
  bool retry=false;
  if(e->sound_id!=q->last_id || e->loop_count!=q->last_loops){
   const uint8_t *bytes=NULL;int length=ops->generate(ops->user,e->sound_id,e->loop_count,&bytes);
   if(length>=0 && add225(ops->now(ops->user),length/22)>add225(q->last_start,q->last_length/22)){
    q->last_length=length;q->last_start=ops->now(ops->user);
    int result=ops->save(ops->user,bytes,length);
    if(result>0){q->last_id=e->sound_id;q->last_loops=e->loop_count;}else retry=result==0;
   }
  }else retry=ops->replay(ops->user)==0;
  if(retry && e->delay_ticks!=-5)e->delay_ticks=-5;
  else {q->count--;for(int j=i;j<q->count;j++)q->entries[j]=q->entries[j+1];i--;}
 }
 return true;
}
