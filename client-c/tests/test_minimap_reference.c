#include "dash3d/minimap.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_int(FILE *f) {
    uint8_t bytes[4]; assert(fread(bytes,1,4,f)==4);
    return (int)((uint32_t)bytes[0]<<24 | (uint32_t)bytes[1]<<16 | (uint32_t)bytes[2]<<8 | bytes[3]);
}
int main(void) {
    setbuf(stdout,NULL);
    Arena arena=arena_create(16*1024*1024);
    Jagfile *archive=jagfile_load_file("../engine/data/pack/client/media",&arena); assert(archive);
    Media media; assert(media_load(&media,archive,&arena)); pix3d_init();
    FILE *f=fopen("tests/fixtures/minimap-reference.bin","rb"); assert(f);
    assert(read_int(f)==0x4d415032); int w=read_int(f),h=read_int(f),cases=read_int(f);
    assert(w==media.mapback->width && h==media.mapback->height && cases==160);
    for(int i=0;i<2048;i++) {
        assert(pix3d_sin_table[i]==read_int(f)); assert(pix3d_cos_table[i]==read_int(f));
    }
    uint32_t *actual=calloc((size_t)w*h,sizeof(*actual)),*expected=calloc((size_t)w*h,sizeof(*expected));
    assert(actual && expected); pix2d_bind(actual,w,h);
    for(int n=0;n<cases;n++) {
        int kind=read_int(f),yaw=read_int(f),px=read_int(f),pz=read_int(f),tx=read_int(f),tz=read_int(f);
        int painted=read_int(f); assert(painted>=0 && painted<=w*h);
        memset(actual,0,(size_t)w*h*sizeof(*actual)); memset(expected,0,(size_t)w*h*sizeof(*expected));
        for(int i=0;i<painted;i++) {int index=read_int(f); assert(index>=0 && index<w*h); expected[index]=(uint32_t)read_int(f);}
        Pix32 *sprite=kind==3?media.mapflag:media.mapdots[kind]; assert(sprite);
        minimap_draw_dot(px,pz,tx,tz,yaw,0,0,sprite,media.mapback);
        for(int i=0;i<w*h;i++) if(actual[i]!=expected[i]) {
            fprintf(stderr,"Java minimap mismatch case=%d sprite=%d yaw=%d pixel=(%d,%d) expected=%06x actual=%06x\n",
                n,kind,yaw,i%w,i/w,expected[i],actual[i]); return 1;
        }
    }
    assert(fgetc(f)==EOF); fclose(f); free(actual); free(expected); arena_free(&arena);
    puts("PASS: 4096 original-Java trig entries and all pixels in 160 minimap sprite frames, including quantization, rotation, crop offsets, distant masks and radius rejection");
    return 0;
}
