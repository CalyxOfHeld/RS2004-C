#ifndef FLO_TYPE_H
#define FLO_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"

typedef struct FloType {
    int rgb;
    int texture;
    bool overlay;
    bool occlude;
    char name[64];
    int hue;
    int saturation;
    int lightness;
    int chroma;
    int luminance;
    int hsl;
} FloType;

typedef struct FloConfig {
    int count;
    FloType* instances;
} FloConfig;

void flo_unpack(FloConfig* config, Jagfile* jag);
void flo_decode(FloType* flo, Packet* pkt);
void flo_set_color(FloType* flo, int rgb);
int flo_hsl24to16(int h, int s, int l);
void flo_free(FloConfig* config);

#endif // FLO_TYPE_H
