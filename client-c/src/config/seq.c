#include "config/seq.h"
#include "gfx/anim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void seq_unpack(SeqConfig* config, Jagfile* jag) {
    size_t len = 0;
    uint8_t* data = jagfile_read(jag, "seq.dat", NULL, &len, NULL);
    if (!data) {
        config->count = 0;
        config->instances = NULL;
        return;
    }

    Packet pkt = packet_wrap(data, len);

    config->count = g2(&pkt);
    config->instances = (SeqType*)calloc(config->count, sizeof(SeqType));

    for (int i = 0; i < config->count; i++) {
        SeqType* seq = &config->instances[i];
        seq->replayoff = -1;
        seq->priority = 5;
        seq->righthand = -1;
        seq->lefthand = -1;
        seq->replaycount = 99;
        seq_decode(seq, &pkt);
    }

    free(data);
}

void seq_decode(SeqType* seq, Packet* pkt) {
    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            if (seq->frame_count == 0) {
                seq->frame_count = 1;
                seq->frames = (int*)malloc(sizeof(int));
                seq->frames[0] = -1;
                seq->iframes = (int*)malloc(sizeof(int));
                seq->iframes[0] = -1;
                seq->delay = (int*)malloc(sizeof(int));
                seq->delay[0] = -1;
            }
            return;
        }

        if (code == 1) {
            seq->frame_count = g1(pkt);
            seq->frames = (int*)malloc(seq->frame_count * sizeof(int));
            seq->iframes = (int*)malloc(seq->frame_count * sizeof(int));
            seq->delay = (int*)malloc(seq->frame_count * sizeof(int));

            for (int i = 0; i < seq->frame_count; i++) {
                seq->frames[i] = g2(pkt);
                seq->iframes[i] = g2(pkt);
                if (seq->iframes[i] == 65535) {
                    seq->iframes[i] = -1;
                }
                seq->delay[i] = g2(pkt);
                // SeqType.decode resolves the delay at decode time, so
                // AnimFrame must already be unpacked (client.load order).
                if (seq->delay[i] == 0) {
                    AnimFrame *af = anim_get_frame(seq->frames[i]);
                    if (af) seq->delay[i] = af->delay;
                }
                if (seq->delay[i] == 0) {
                    seq->delay[i] = 1;
                }
            }
        } else if (code == 2) {
            seq->replayoff = g2(pkt);
        } else if (code == 3) {
            int count = g1(pkt);
            seq->walkmerge_count = count;
            seq->walkmerge = (int*)malloc((count + 1) * sizeof(int));
            for (int i = 0; i < count; i++) {
                seq->walkmerge[i] = g1(pkt);
            }
            seq->walkmerge[count] = 9999999;
        } else if (code == 4) {
            seq->stretches = true;
        } else if (code == 5) {
            seq->priority = g1(pkt);
        } else if (code == 6) {
            seq->righthand = g2(pkt);
        } else if (code == 7) {
            seq->lefthand = g2(pkt);
        } else if (code == 8) {
            seq->replaycount = g1(pkt);
        } else {
            fprintf(stderr, "Error unrecognised seq config code: %d\n", code);
        }
    }
}

void seq_free(SeqConfig* config) {
    if (!config || !config->instances) return;
    for (int i = 0; i < config->count; i++) {
        SeqType* seq = &config->instances[i];
        if (seq->frames) free(seq->frames);
        if (seq->iframes) free(seq->iframes);
        if (seq->delay) free(seq->delay);
        if (seq->walkmerge) free(seq->walkmerge);
    }
    free(config->instances);
    config->instances = NULL;
    config->count = 0;
}

int seq_get_delay(const SeqType* seq, int frame_idx) {
    // Java reads seq.delay[frame] directly; seq_decode resolves it like SeqType.decode.
    if (!seq || !seq->delay || frame_idx < 0 || frame_idx >= seq->frame_count) return 1;
    return seq->delay[frame_idx];
}
