#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "core/wordpack.h"
#include "core/packet.h"

static int wp_read_int(FILE *f) {
    unsigned char b[4];assert(fread(b,1,4,f)==4);
    return (int)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);
}
static size_t wp_read_bytes(FILE *f,unsigned char *out,size_t capacity) {
    int n=wp_read_int(f);assert(n>=0 && (size_t)n<=capacity);
    assert(fread(out,1,(size_t)n,f)==(size_t)n);return (size_t)n;
}
static void wp_guards(const unsigned char *b,size_t length,size_t start,size_t used,unsigned char marker) {
    for(size_t i=0;i<length;i++)if(i<start || i>=start+used)assert(b[i]==marker);
}
static void wordpack_reference(void) {
    const char *path=getenv("RS2_WORDPACK_ORACLE");
    FILE *f=fopen(path && *path?path:"tests/fixtures/wordpack-reference.bin","rb");
    if(!f && (!path || !*path))f=fopen("client-c/tests/fixtures/wordpack-reference.bin","rb");
    assert(f);assert(wp_read_int(f)==0x57503232 && wp_read_int(f)==1);
    int packs=wp_read_int(f);assert(packs>130000);
    for(int i=0;i<packs;i++) {
        bool flush=wp_read_int(f)!=0;
        unsigned char input[256],expected[256],decoded[128],actual[256],text[128];
        size_t length=wp_read_bytes(f,input,sizeof(input));
        size_t packed=wp_read_bytes(f,expected,sizeof(expected));
        size_t decoded_len=wp_read_bytes(f,decoded,sizeof(decoded));
        memset(actual,0xa5,sizeof(actual));Packet p=packet_wrap(actual,packed+3);p.pos=3;
        assert(wordpack_pack(&p,(const char*)input,length,flush));
        if(p.pos!=packed+3 || memcmp(actual+3,expected,packed))fprintf(stderr,"WordPack original pack case%d differs\n",i);
        assert(p.pos==packed+3 && !p.error && !memcmp(actual+3,expected,packed));
        wp_guards(actual,sizeof(actual),3,packed,0xa5);
        p.pos=3;memset(text,0x5a,sizeof(text));size_t written=999;
        assert(wordpack_unpack(&p,packed,(char*)text+3,decoded_len+1,&written));
        assert(written==decoded_len && p.pos==packed+3 && !memcmp(text+3,decoded,decoded_len) && text[3+written]==0);
        wp_guards(text,sizeof(text),3,decoded_len+1,0x5a);
        if(i%257==0) {
            p.pos=3;memset(text,0x5a,sizeof(text));written=999;
            assert(!wordpack_unpack(&p,packed,(char*)text+3,decoded_len,&written));
            assert(p.error && p.pos==3 && written==0);wp_guards(text,sizeof(text),0,0,0x5a);
            assert(!wordpack_unpack(&p,packed,(char*)text+3,sizeof(text)-3,&written));
            assert(p.pos==3);wp_guards(text,sizeof(text),0,0,0x5a);
            if(packed) {
                memset(actual,0xa5,sizeof(actual));p=packet_wrap(actual,packed+2);p.pos=3;
                assert(!wordpack_pack(&p,(const char*)input,length,flush) && p.error && p.pos==3);
                wp_guards(actual,sizeof(actual),0,0,0xa5);
            }
        }
    }
    int raw=wp_read_int(f),overflows=0;assert(raw>65000);
    for(int i=0;i<raw;i++) {
        unsigned char input[128],source[256],decoded[128],text[128];
        size_t length=wp_read_bytes(f,input,sizeof(input));bool valid=wp_read_int(f)!=0;
        size_t expected=valid?wp_read_bytes(f,decoded,sizeof(decoded)):0;
        memset(source,0x6b,sizeof(source));memcpy(source+3,input,length);
        memset(text,0x5a,sizeof(text));Packet p=packet_wrap(source,length+3);p.pos=3;size_t written=999;
        bool actual=wordpack_unpack(&p,length,(char*)text+3,sizeof(text)-3,&written);
        if(actual!=valid)fprintf(stderr,"WordPack raw original case%d validity differs\n",i);
        assert(actual==valid);
        if(valid) {
            if(written!=expected || memcmp(text+3,decoded,expected))fprintf(stderr,"WordPack raw original case%d text differs\n",i);
            assert(!p.error && p.pos==length+3 && written==expected && !memcmp(text+3,decoded,expected) && text[3+written]==0);
            wp_guards(text,sizeof(text),3,expected+1,0x5a);
        } else {
            overflows++;assert(p.error && p.pos==3 && written==0);wp_guards(text,sizeof(text),0,0,0x5a);
        }
        assert(!memcmp(source+3,input,length));wp_guards(source,sizeof(source),3,length,0x6b);
    }
    assert(fgetc(f)==EOF);fclose(f);
    printf("PASS WordPack original: %d pack and %d raw decode cases; %d original overflows safely rejected; guards/capacity/sticky errors exact\n",packs,raw,overflows);
}
static void wordpack_bounds(void) {
    unsigned char bytes[8];memset(bytes,0x6b,sizeof(bytes));char output[8];memset(output,'X',sizeof(output));
    Packet p=packet_wrap(bytes,1);p.pos=1;size_t count=99;
    assert(!wordpack_unpack(&p,1,output,sizeof(output),&count) && p.error && p.pos==1 && count==0);
    for(size_t i=0;i<sizeof(output);i++)assert(output[i]=='X');
    p=packet_wrap(bytes,sizeof(bytes));assert(!wordpack_pack(&p,NULL,1,true) && p.error && p.pos==0);
    wp_guards(bytes,sizeof(bytes),0,0,0x6b);
    assert(!wordpack_pack(&p,"e",1,true) && p.error && p.pos==0);
    wp_guards(bytes,sizeof(bytes),0,0,0x6b);
    p=packet_wrap(bytes,sizeof(bytes));p.pos=sizeof(bytes)+1;
    assert(!wordpack_pack(&p,"e",1,true) && p.error && p.pos==sizeof(bytes)+1);
    p=packet_wrap(NULL,0);assert(wordpack_pack(&p,NULL,0,true) && p.pos==0 && !p.error);
    assert(wordpack_unpack(&p,0,output,1,&count) && count==0 && output[0]==0);
    p=packet_wrap(bytes,sizeof(bytes));assert(!wordpack_unpack(&p,0,NULL,1,&count) && p.error);
    assert(!wordpack_pack(NULL,"e",1,true));assert(!wordpack_unpack(NULL,0,output,sizeof(output),&count));
    p=packet_wrap(NULL,1);assert(!wordpack_pack(&p,"xx",2,true) && p.error);
    p=packet_wrap(NULL,1);assert(!wordpack_unpack(&p,1,output,sizeof(output),&count) && p.error);
    puts("PASS WordPack malformed/capacity/NULL/empty input boundaries are transactional");
}

int main(void) {
    printf("[Test Packet] Starting Packet unit tests...\n");
    wordpack_reference();
    wordpack_bounds();

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
