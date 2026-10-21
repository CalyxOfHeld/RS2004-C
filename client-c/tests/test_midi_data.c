#include "audio/midi.h"
#include "core/bzip2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_int(FILE *f) {
    uint8_t b[4];assert(fread(b,1,4,f)==4);
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
int main(void) {
    setbuf(stdout,NULL);
    FILE *f=fopen("tests/fixtures/midi-reference.bin","rb");assert(f);
    assert(read_int(f)==0x4d494431);uint32_t count=read_int(f);assert(count==230);
    size_t total=0;
    for(uint32_t i=0;i<count;i++) {
        int hi=fgetc(f),lo=fgetc(f);assert(hi>=0 && lo>=0);size_t n=(size_t)hi*256+lo;
        char name[128],path[256];assert(n<sizeof(name));assert(fread(name,1,n,f)==n);name[n]=0;
        uint32_t size=read_int(f),crc=read_int(f),length=read_int(f);
        assert(size<=MIDI_MAX_PACKED && length<=MIDI_MAX_DECODED);
        uint8_t *expected=malloc(length),*packed=malloc(size);assert(expected && packed);
        assert(fread(expected,1,length,f)==length);
        snprintf(path,sizeof(path),"../engine/data/pack/client/%s",name);
        FILE *asset=fopen(path,"rb");assert(asset);assert(fread(packed,1,size,asset)==size && fgetc(asset)==EOF);fclose(asset);
        uint8_t *decoded=NULL;size_t actual=0;
        assert(midi_unpack(packed,size,crc,size,&decoded,&actual));
        assert(actual==length && !memcmp(expected,decoded,length));free(decoded);total+=actual;
        assert(!midi_unpack(packed,size,crc^1,size,&decoded,&actual) && !decoded && !actual);
        assert(!midi_unpack(packed,size,crc,size+1,&decoded,&actual));
        if(i==0) {
            uint8_t *guard=malloc(length+32);assert(guard);memset(guard,0xa5,length+32);
            assert(!bzip_decompress_exact(guard+16,length-1,packed+4,size-4));
            for(size_t k=0;k<16;k++) assert(guard[k]==0xa5);
            for(size_t k=16+length-1;k<length+32;k++) assert(guard[k]==0xa5);
            assert(!bzip_decompress_exact(guard+16,length+1,packed+4,size-4));
            assert(!bzip_decompress_exact(guard+16,length,packed+4,size-5));
            packed[size-2]^=1;
            assert(!midi_unpack(packed,size,12345678u,size,&decoded,&actual)); // stream CRC still required
            packed[size-2]^=1;
            uint8_t first=packed[0];packed[0]=0x7f;assert(!midi_unpack(packed,size,0,0,&decoded,&actual));packed[0]=first;
            for(uint32_t mutation=0;mutation<128;mutation++) {
                size_t index=4+(mutation*7919u)%(size-4);uint8_t bit=(uint8_t)(1u<<(mutation%8));
                packed[index]^=bit;memset(guard,0xa5,length+32);
                bool accepted=bzip_decompress_exact(guard+16,length,packed+4,size-4);
                if(accepted) assert(!memcmp(guard+16,expected,length));
                for(size_t k=0;k<16;k++) assert(guard[k]==0xa5 && guard[16+length+k]==0xa5);
                packed[index]^=bit;
            }
            assert(midi_unpack(packed,size,crc,size,&decoded,&actual));free(decoded);
            free(guard);
        }
        free(expected);free(packed);
    }
    assert(fgetc(f)==EOF);fclose(f);
    f=fopen("tests/fixtures/bzip-boundaries.bin","rb");assert(f);
    assert(read_int(f)==0x425a5031 && read_int(f)==6);
    for(int i=0;i<6;i++) {
        size_t size=read_int(f),compressed=read_int(f);assert(size<300000 && compressed<300000);
        uint8_t *packed=malloc(compressed),*expected=malloc(size?size:1),*guard=malloc(size+32);assert(packed && expected && guard);
        assert(fread(packed,1,compressed,f)==compressed && fread(expected,1,size,f)==size);
        memset(guard,0xa5,size+32);
        assert(bzip_decompress_exact(guard+16,size,packed,compressed));
        assert(!memcmp(guard+16,expected,size));
        for(size_t k=0;k<16;k++) assert(guard[k]==0xa5 && guard[size+16+k]==0xa5);
        if(size) assert(!bzip_decompress_exact(guard+16,size-1,packed,compressed));
        assert(!bzip_decompress_exact(guard+16,size+1,packed,compressed));
        free(packed);free(expected);free(guard);
    }
    assert(fgetc(f)==EOF);fclose(f);
    const uint8_t bad[]={0,0,0,4,0,0,0,0};uint8_t *decoded;size_t length;
    assert(!midi_unpack(bad,sizeof(bad),0,0,&decoded,&length));
    assert(!midi_unpack(NULL,0,0,0,&decoded,&length));
    assert(!midi_play_jingle(bad,sizeof(bad),INT_MAX,20));
    assert(!midi_delay_step(NULL));
    for (unsigned initial=0;initial<=65535;initial++) {
        uint16_t delay=(uint16_t)initial;
        unsigned elapsed=0;
        while(delay) { bool resume=midi_delay_step(&delay);elapsed++;assert(resume==(delay==0)); }
        assert(elapsed==(initial+19)/20);
        assert(!midi_delay_step(&delay));
    }
    puts("PASS: all 65536 original g2 jingle delays expire once at 20 ms logic steps; zero never schedules restart");
    assert(!midi_play("../adventure",0,0));
    printf("PASS: %u actual cache songs/jingles match %zu original Java bytes; bounded writes, exact length, CRC/size, truncation, corruption, RIFF and recovery\n",count,total);
    puts("PASS: 128 mutated streams preserve canaries and recover; 0/1/4095/4096/4097/220123-byte independent BZip2 boundary/multiblock streams");
    return 0;
}
