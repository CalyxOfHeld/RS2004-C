#ifndef RS2_SOUND_QUEUE_H
#define RS2_SOUND_QUEUE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {int sound_id,loop_count,delay_ticks;} AudioQueuedSound225;
typedef struct {AudioQueuedSound225 entries[50];int count,last_id,last_loops,last_length;int64_t last_start;} SoundQueue225;
typedef struct {
 void *user;
 int (*generate)(void*,int,int,const uint8_t**); /* WAV length,or -1 for null/exception. */
 int64_t (*now)(void*);
 int (*save)(void*,const uint8_t*,int); /* 1 accepted,0 busy,-1 exception. */
 int (*replay)(void*);
} SoundQueueOps225;
void sound_queue_init225(SoundQueue225 *q);
bool sound_queue_add225(SoundQueue225 *q,int id,int loops,int delay);
bool sound_queue_tick225(SoundQueue225 *q,const SoundQueueOps225 *ops);
#endif
