#ifndef VARP_TYPE_H
#define VARP_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"

// VarpType (varp.dat). Only clientcode is read by client.java (updateVarp,
// client.java:4486); the other codes are decoded and kept for parity.
typedef struct VarpType {
    int code1;
    int code2;
    bool has_code3;
    bool code4;
    int clientcode;
    bool code6;
    int code7;
    bool code8;
    bool has_code10; // Java code10 != null
    char code10[128];
} VarpType;

typedef struct VarpTypeConfig {
    int count;
    VarpType *instances;
    int code3_count;
    int *code3;
} VarpTypeConfig;

bool varp_type_unpack(VarpTypeConfig *config, Jagfile *jag);
void varp_type_decode(VarpTypeConfig *config, VarpType *varp, int id, Packet *pkt);
// clientcode of varp id, 0 when id is outside the table.
int varp_type_clientcode(const VarpTypeConfig *config, int id);
void varp_type_free(VarpTypeConfig *config);

#endif // VARP_TYPE_H
