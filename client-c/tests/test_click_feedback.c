#include "ui/click_feedback.h"
#include "gfx/pix2d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t read_int(FILE *f) {
    uint8_t b[4];assert(fread(b,1,4,f)==4);
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
int main(void) {
    Arena arena=arena_create(16*1024*1024);
    Jagfile *archive=jagfile_load_file("../engine/data/pack/client/media",&arena);assert(archive);
    Media media;assert(media_load(&media,archive,&arena));
    FILE *f=fopen("tests/fixtures/cross-reference.bin","rb");assert(f);
    assert(read_int(f)==0x43525331 && read_int(f)==24);
    size_t count=789*532;uint32_t *pixels=calloc(count,4),*expected=calloc(count,4);assert(pixels && expected);
    pix2d_bind(pixels,789,532);
    for(int n=0;n<24;n++) {
        int frame=read_int(f),x=read_int(f),y=read_int(f),lit=read_int(f);
        memset(pixels,0,count*4);memset(expected,0,count*4);
        for(int i=0;i<lit;i++) {uint32_t index=read_int(f);assert(index<count);expected[index]=read_int(f);}
        ClickFeedback cross={frame<4?CLICK_WALK:CLICK_ACTION,(frame%4)*100,x,y};
        click_feedback_draw(&cross,&media,8,11,512,334);
        assert(!memcmp(pixels,expected,count*4));
        assert(pix2d_clip_left==0 && pix2d_clip_right==789 && pix2d_clip_top==0 && pix2d_clip_bottom==532);
    }
    assert(fgetc(f)==EOF);fclose(f);
    for(int mode=CLICK_WALK;mode<=CLICK_ACTION;mode++) {
        ClickFeedback cross={0};click_feedback_start(&cross,mode,200,200);
        for(int tick=0;tick<20;tick++) {
            assert(click_feedback_frame(&cross)==tick/5+(mode==CLICK_ACTION?4:0));
            click_feedback_tick(&cross);
        }
        assert(cross.mode==CLICK_NONE && click_feedback_frame(&cross)==-1);
    }
    free(pixels);free(expected);arena_free(&arena);
    puts("PASS: original yellow/red cross pixels and viewport clipping in 24 frames; four 100 ms frames and 400 ms expiration at 50 Hz");
    return 0;
}
