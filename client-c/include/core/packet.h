#ifndef RS2_PACKET_H
#define RS2_PACKET_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/isaac.h"

typedef struct {
    uint8_t *data;
    size_t pos;
    size_t length;
    size_t bit_pos;
    Isaac *random;
    bool error; /* Sticky bounds/format error; consumers must reject malformed data. */
} Packet;

Packet packet_wrap(uint8_t *buffer, size_t length);

// Write methods (matching Packet.java exactly)
void p1isaac(Packet *pkt, int32_t val);
void p1(Packet *pkt, int32_t val);
void p2(Packet *pkt, int32_t val);
void ip2(Packet *pkt, int32_t val);
void p3(Packet *pkt, int32_t val);
void p4(Packet *pkt, int32_t val);
void ip4(Packet *pkt, int32_t val);
void p8(Packet *pkt, int64_t val);
void pjstr(Packet *pkt, const char *str);
void pdata(Packet *pkt, const uint8_t *src, size_t len);
void psize1(Packet *pkt, size_t size);

// Read methods
int32_t g1(Packet *pkt);
int8_t  g1b(Packet *pkt);
int32_t g2(Packet *pkt);
int16_t g2b(Packet *pkt);
int32_t g3(Packet *pkt);
int32_t g4(Packet *pkt);
int64_t g8(Packet *pkt);
void    gjstr(Packet *pkt, char *dst, size_t max_len);
void    gdata(Packet *pkt, uint8_t *dst, size_t len);
int32_t gsmart(Packet *pkt);
int32_t gsmarts(Packet *pkt);

// Bitstream methods
void    access_bits(Packet *pkt);
void    access_bytes(Packet *pkt);
int32_t gbit(Packet *pkt, int32_t n);

// CRC32
uint32_t packet_crc32(const uint8_t *data, size_t len);

#endif // RS2_PACKET_H
