#include "dash3d/world.h"
#include "dash3d/world3d.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
int main(void) {
 World *w=calloc(1,sizeof(*w));World3D *s=calloc(1,sizeof(*s));assert(w && s);
 world_init(w,3,3);world3d_init(s,3,3,4);
 FILE *f=fopen("tests/fixtures/bridge-level-reference.bin","rb");assert(f);
 for(int l=0;l<4;l++)for(int b=0;b<256;b++)for(int flag=0;flag<256;flag++) {
  w->tile_flags[1][1][1]=(unsigned char)b;w->tile_flags[l][1][1]=(unsigned char)flag;
  world_build(w,s,NULL,NULL);assert(s->tiles[l][1][1].draw_level==fgetc(f));
 }
 assert(fgetc(f)==EOF);fclose(f);world3d_dispose(s);free(s);free(w);
 puts("PASS: 262144 original bridge/source-plane/visibility draw-level cases");return 0;
}
