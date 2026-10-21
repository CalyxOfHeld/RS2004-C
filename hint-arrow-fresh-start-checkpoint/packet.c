#include "core/packet.h"
#include <string.h>

static const uint32_t BITMASK[33] = {
    0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767,
    65535, 131071, 262143, 524287, 1048575, 2097151, 4194303, 8388607, 16777215,
    33554431, 67108863, 134217727, 268435455, 536870911, 1073741823, 2147483647,
    0xFFFFFFFF
};

Packet packet_wrap(uint8_t *buffer, size_t length) {
    Packet pkt;
    pkt.data = buffer;
    pkt.pos = 0;
    pkt.length = length;
    pkt.bit_pos = 0;
    pkt.random = NULL;
    pkt.error = false;
    return pkt;
}

void p1isaac(Packet *pkt, int32_t val) {
    uint32_t rand_val = pkt->random ? isaac_next(pkt->random) : 0;
    pkt->data[pkt->pos++] = (uint8_t)(val + rand_val);
}

void p1(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)val;
}

void p2(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
    pkt->data[pkt->pos++] = (uint8_t)val;
}

void ip2(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)val;
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
}

void p3(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)(val >> 16);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
    pkt->data[pkt->pos++] = (uint8_t)val;
}

void p4(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)(val >> 24);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 16);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
    pkt->data[pkt->pos++] = (uint8_t)val;
}

void ip4(Packet *pkt, int32_t val) {
    pkt->data[pkt->pos++] = (uint8_t)val;
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 16);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 24);
}

void p8(Packet *pkt, int64_t val) {
    pkt->data[pkt->pos++] = (uint8_t)(val >> 56);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 48);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 40);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 32);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 24);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 16);
    pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
    pkt->data[pkt->pos++] = (uint8_t)val;
}

void pjstr(Packet *pkt, const char *str) {
    size_t len = strlen(str);
    memcpy(&pkt->data[pkt->pos], str, len);
    pkt->pos += len;
    pkt->data[pkt->pos++] = 10; // Jagex string delimiter is newline (\n / 10)
}

void pdata(Packet *pkt, const uint8_t *src, size_t len) {
    memcpy(&pkt->data[pkt->pos], src, len);
    pkt->pos += len;
}

void psize1(Packet *pkt, size_t size) {
    pkt->data[pkt->pos - size - 1] = (uint8_t)size;
}

static bool packet_require(Packet *pkt,size_t bytes) {
    if(!pkt || pkt->error) return false;
    if(pkt->pos>pkt->length || bytes>pkt->length-pkt->pos || (bytes && !pkt->data)) {
        pkt->error=true;return false;
    }
    return true;
}

int32_t g1(Packet *pkt) {
    if(!packet_require(pkt,1)) return 0;
    return pkt->data[pkt->pos++] & 0xFF;
}

int8_t g1b(Packet *pkt) {
    if(!packet_require(pkt,1)) return 0;
    return (int8_t)pkt->data[pkt->pos++];
}

int32_t g2(Packet *pkt) {
    if(!packet_require(pkt,2)) return 0;
    pkt->pos += 2;
    return ((pkt->data[pkt->pos - 2] & 0xFF) << 8) | (pkt->data[pkt->pos - 1] & 0xFF);
}

int16_t g2b(Packet *pkt) {
    if(!packet_require(pkt,2)) return 0;
    pkt->pos += 2;
    int32_t val = ((pkt->data[pkt->pos - 2] & 0xFF) << 8) | (pkt->data[pkt->pos - 1] & 0xFF);
    if (val > 32767) {
        val -= 65536;
    }
    return (int16_t)val;
}

int32_t g3(Packet *pkt) {
    if(!packet_require(pkt,3)) return 0;
    pkt->pos += 3;
    return ((pkt->data[pkt->pos - 3] & 0xFF) << 16) |
           ((pkt->data[pkt->pos - 2] & 0xFF) << 8) |
           (pkt->data[pkt->pos - 1] & 0xFF);
}

int32_t g4(Packet *pkt) {
    if(!packet_require(pkt,4)) return 0;
    pkt->pos += 4;
    return ((pkt->data[pkt->pos - 4] & 0xFF) << 24) |
           ((pkt->data[pkt->pos - 3] & 0xFF) << 16) |
           ((pkt->data[pkt->pos - 2] & 0xFF) << 8) |
           (pkt->data[pkt->pos - 1] & 0xFF);
}

int64_t g8(Packet *pkt) {
    int64_t high = (uint32_t)g4(pkt);
    int64_t low = (uint32_t)g4(pkt);
    return (high << 32) | low;
}

void gjstr(Packet *pkt, char *dst, size_t max_len) {
    size_t i = 0;
    while (pkt->pos < pkt->length && pkt->data[pkt->pos] != 10) {
        if (dst && i + 1 < max_len) {
            dst[i++] = (char)pkt->data[pkt->pos];
        }
        pkt->pos++;
    }
    if (pkt->pos < pkt->length && pkt->data[pkt->pos] == 10) {
        pkt->pos++; // skip delimiter
    }
    else pkt->error=true;
    if(max_len && dst) dst[i] = '\0';
}

void gdata(Packet *pkt, uint8_t *dst, size_t len) {
    if(!packet_require(pkt,len)) return;
    memcpy(dst, &pkt->data[pkt->pos], len);
    pkt->pos += len;
}

int32_t gsmart(Packet *pkt) {
    if(!packet_require(pkt,1)) return 0;
    int32_t peek = pkt->data[pkt->pos] & 0xFF;
    return (peek < 128) ? (g1(pkt) - 64) : (g2(pkt) - 49152);
}

int32_t gsmarts(Packet *pkt) {
    if(!packet_require(pkt,1)) return 0;
    int32_t peek = pkt->data[pkt->pos] & 0xFF;
    return (peek < 128) ? g1(pkt) : (g2(pkt) - 32768);
}

void access_bits(Packet *pkt) {
    pkt->bit_pos = pkt->pos * 8;
}

void access_bytes(Packet *pkt) {
    pkt->pos = (pkt->bit_pos + 7) / 8;
}

int32_t gbit(Packet *pkt, int32_t n) {
    if(!pkt || pkt->error) return 0;
    if(n<0 || n>32 || pkt->bit_pos>pkt->length*8 || (size_t)n>pkt->length*8-pkt->bit_pos) {
        pkt->error=true;return 0;
    }
    if(!n) return 0;
    size_t byte_idx = pkt->bit_pos >> 3;
    int32_t bit_rem = 8 - (int32_t)(pkt->bit_pos & 7);
    int32_t val = 0;
    pkt->bit_pos += n;

    while (n > bit_rem) {
        val += (pkt->data[byte_idx++] & BITMASK[bit_rem]) << (n - bit_rem);
        n -= bit_rem;
        bit_rem = 8;
    }

    if (n == bit_rem) {
        val += pkt->data[byte_idx] & BITMASK[bit_rem];
    } else {
        val += (pkt->data[byte_idx] >> (bit_rem - n)) & BITMASK[n];
    }

    return val;
}

uint32_t packet_crc32(const uint8_t *data, size_t len) {
    static uint32_t crc_table[256];
    static bool table_init = false;
    if (!table_init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                if (c & 1) {
                    c = (c >> 1) ^ 0xEDB88320;
                } else {
                    c >>= 1;
                }
            }
            crc_table[i] = c;
        }
        table_init = true;
    }

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}
