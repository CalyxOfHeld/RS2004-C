#ifndef SEQ_TYPE_H
#define SEQ_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"

typedef struct SeqType {
    int frame_count;
    int* frames;
    int* iframes;
    int* delay;
    int replayoff;
    int* walkmerge;
    int walkmerge_count;
    bool stretches;
    int priority;
    int righthand;
    int lefthand;
    int replaycount;
} SeqType;

typedef struct SeqConfig {
    int count;
    SeqType* instances;
} SeqConfig;

void seq_unpack(SeqConfig* config, Jagfile* jag);
void seq_decode(SeqType* seq, Packet* pkt);
void seq_free(SeqConfig* config);
int seq_get_delay(const SeqType* seq, int frame_idx);

#endif // SEQ_TYPE_H
