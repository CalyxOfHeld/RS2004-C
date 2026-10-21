#define SDL_MAIN_HANDLED
#include "audio/wave.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned read_be(FILE *file,int bytes) {
    unsigned value=0;
    while(bytes--) {int c=fgetc(file);assert(c!=EOF);value=(value<<8)|(unsigned)c;}
    return value;
}
int main(int argc,char **argv) {
    setbuf(stdout,NULL);
    if(argc!=2) {fprintf(stderr,"Supply the original Java WaveOracle binary fixture\n");return 2;}
    Arena arena=arena_create(32*1024*1024);
    Jagfile *cache=jagfile_load_file("../engine/data/pack/client/sounds",&arena);
    if(!cache) cache=jagfile_load_file("engine/data/pack/client/sounds",&arena);
    assert(cache && wave_unpack(cache));
    FILE *fixture=fopen(argv[1],"rb");assert(fixture);
    assert(read_be(fixture,4)==0x57323235);
    int cases=0,failures=0;size_t total=0;
    for(;;) {
        unsigned id=read_be(fixture,2);if(id==65535) break;
        unsigned loops=read_be(fixture,1),size=read_be(fixture,4);
        printf("Compare track=%u loops=%u bytes=%u\n",id,loops,size);
        assert(size<=441000);
        unsigned char *expected=malloc(size);assert(expected);
        assert(fread(expected,1,size,fixture)==size);
        size_t pcm=0,bytes=0;
        unsigned char *actual=wave_generate((int)id,(int)loops,&pcm,&bytes);
        size_t differences=0,first=size;
        if(actual && bytes==size) for(size_t i=0;i<size;i++) {
            if(actual[i]!=expected[i]) {if(first==size) first=i;differences++;}
        }
        if(!actual || bytes!=size || differences) {
            printf("FAIL track=%u loops=%u bytes=%zu expected=%u differences=%zu first=%zu\n",id,loops,bytes,size,differences,first);failures++;
        }
        free(expected);total+=size;cases++;
    }
    fclose(fixture);
    // Corrupt actual cache bytes, preserving the real archive lookup path.
    size_t dat_size=0;
    unsigned char *dat=jagfile_read(cache,"sounds.dat",NULL,&dat_size,NULL);
    assert(dat && dat_size>2);
    uint32_t hash=0;
    const char *name="SOUNDS.DAT";
    for(const char *p=name;*p;p++) hash=hash*61+(unsigned char)*p-32;
    int32_t file_hash=(int32_t)hash,size=(int32_t)dat_size,offset=0;
    Jagfile malformed={.buffer=dat,.file_count=1,.file_hash=&file_hash,
        .file_unpacked_size=&size,.file_packed_size=&size,.file_offset=&offset,.unpacked=true};
    unsigned char first[2]={dat[0],dat[1]};dat[0]=3;dat[1]=232; // invalid track 1000
    assert(!wave_unpack(&malformed));
    dat[0]=first[0];dat[1]=first[1];size--;
    assert(!wave_unpack(&malformed));
    assert(wave_unpack(cache));
    free(dat);wave_free_all();arena_free(&arena);
    puts("PASS: out-of-range cache track and truncated stream rejected; original cache reloads");
    printf("Java WAV oracle: %d records, %zu bytes, %d mismatches; identical noise-table input only\n",cases,total,failures);
    return failures?1:0;
}
