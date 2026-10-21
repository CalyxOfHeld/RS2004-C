#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "core/packet.h"

int main(void) {
    printf("[Test Packet] Starting Packet unit tests...\n");

    uint8_t buf[256];
    memset(buf, 0, sizeof(buf));
    Packet pkt = packet_wrap(buf, sizeof(buf));

    // 1. Primitive writes
    p1(&pkt, 0x12);
    p2(&pkt, 0x3456);
    ip2(&pkt, 0x789A); // little endian: 0x9A, 0x78
    p3(&pkt, 0xBCDEF0);
    p4(&pkt, 0x11223344);
    ip4(&pkt, 0x55667788); // little endian: 0x88, 0x77, 0x66, 0x55
    pjstr(&pkt, "hello world");

    // 2. Primitive reads
    pkt.pos = 0;
    assert(g1(&pkt) == 0x12);
    assert(g2(&pkt) == 0x3456);

    // Verify ip2
    int32_t b0 = g1(&pkt);
    int32_t b1 = g1(&pkt);
    assert(b0 == 0x9A && b1 == 0x78);

    assert(g3(&pkt) == 0xBCDEF0);
    assert(g4(&pkt) == 0x11223344);

    // Verify ip4
    int32_t ib0 = g1(&pkt);
    int32_t ib1 = g1(&pkt);
    int32_t ib2 = g1(&pkt);
    int32_t ib3 = g1(&pkt);
    assert(ib0 == 0x88 && ib1 == 0x77 && ib2 == 0x66 && ib3 == 0x55);

    char str[32];
    gjstr(&pkt, str, sizeof(str));
    assert(strcmp(str, "hello world") == 0);

    // 3. Smart integers
    pkt.pos = 0;
    // gsmart < 128
    p1(&pkt, 74); // 74 - 64 = 10
    // gsmart >= 128
    p2(&pkt, 49152 + 500); // 49152 + 500 - 49152 = 500
    pkt.pos = 0;
    assert(gsmart(&pkt) == 10);
    assert(gsmart(&pkt) == 500);

    // 4. Bitstream test
    pkt.pos = 0;
    // Write 3 bytes: 0b10101010, 0b11110000, 0b00001111
    p1(&pkt, 0xAA);
    p1(&pkt, 0xF0);
    p1(&pkt, 0x0F);

    pkt.pos = 0;
    access_bits(&pkt);
    // Read: 1 bit (1), 3 bits (010 = 2), 4 bits (1010 = 10) -> totals first byte 0xAA
    assert(gbit(&pkt, 1) == 1);
    assert(gbit(&pkt, 3) == 2);
    assert(gbit(&pkt, 4) == 10);
    // Read: 8 bits across byte 2 (0xF0)
    assert(gbit(&pkt, 8) == 0xF0);
    // Read: 8 bits across byte 3 (0x0F)
    assert(gbit(&pkt, 8) == 0x0F);
    access_bytes(&pkt);
    assert(pkt.pos == 3);

    uint8_t truncated[]={0xff};
    Packet bad=packet_wrap(truncated,sizeof(truncated));
    assert(g2(&bad)==0 && bad.error && bad.pos==0);
    bad=packet_wrap(truncated,sizeof(truncated));access_bits(&bad);
    assert(gbit(&bad,9)==0 && bad.error && bad.bit_pos==0);
    bad=packet_wrap(truncated,sizeof(truncated));char no_string[4];
    gjstr(&bad,no_string,sizeof(no_string));assert(bad.error);
    puts("  Truncated byte/bit/string reads report bounds errors without overread");

    printf("[Test Packet] ALL TESTS PASSED!\n");
    return 0;
}
