#include "config/varp_type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// VarpType.unpack / decode (jagex2/config/VarpType.java).
bool varp_type_unpack(VarpTypeConfig *config, Jagfile *jag) {
    size_t len = 0;
    uint8_t *data = jagfile_read(jag, "varp.dat", NULL, &len, NULL);
    memset(config, 0, sizeof(*config));
    if (!data) return false;

    Packet pkt = packet_wrap(data, len);
    config->count = g2(&pkt);
    config->instances = (VarpType *)calloc((size_t)config->count, sizeof(VarpType));
    config->code3 = (int *)calloc((size_t)config->count, sizeof(int));
    if (!config->instances || !config->code3) {
        free(data);
        varp_type_free(config);
        return false;
    }
    for (int i = 0; i < config->count; i++) {
        VarpType *v = &config->instances[i];
        v->code4 = true;
        varp_type_decode(config, v, i, &pkt);
    }
    free(data);
    return true;
}

void varp_type_decode(VarpTypeConfig *config, VarpType *varp, int id, Packet *pkt) {
    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            return;
        }
        if (code == 1) {
            varp->code1 = g1(pkt);
        } else if (code == 2) {
            varp->code2 = g1(pkt);
        } else if (code == 3) {
            varp->has_code3 = true;
            config->code3[config->code3_count++] = id;
        } else if (code == 4) {
            varp->code4 = false;
        } else if (code == 5) {
            varp->clientcode = g2(pkt);
        } else if (code == 6) {
            varp->code6 = true;
        } else if (code == 7) {
            varp->code7 = g4(pkt);
        } else if (code == 8) {
            varp->code8 = true;
        } else if (code == 10) {
            gjstr(pkt, varp->code10, sizeof(varp->code10));
            varp->has_code10 = true;
        } else {
            printf("Error unrecognised config code: %d\n", code);
        }
    }
}

int varp_type_clientcode(const VarpTypeConfig *config, int id) {
    if (!config || !config->instances || id < 0 || id >= config->count) return 0;
    return config->instances[id].clientcode;
}

void varp_type_free(VarpTypeConfig *config) {
    if (!config) return;
    free(config->instances);
    free(config->code3);
    memset(config, 0, sizeof(*config));
}
