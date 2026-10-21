#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"

static uint32_t fb[64*64], saved[64*64];
static Pix3DVertex vertex(double sx,double sy,double z,double shade) {
    return (Pix3DVertex){(sx-32)*z/512,(sy-32)*z/512,z,0,0,shade};
}
static void triangle(double z,int col,int alpha) {
    pix3d_scene_triangle(vertex(8,8,z,col),vertex(32,56,z,col),vertex(56,8,z,col),-1,alpha);
}
int main(void) {
    pix2d_bind(fb,64,64); pix3d_init(); pix3d_init_colour_table(0.8); pix3d_init_2d();
    pix3d_colour_table[1]=0xff0000; pix3d_colour_table[2]=0x0000ff;
    pix3d_begin_scene(); triangle(100,1,0); triangle(300,2,0); pix3d_end_scene();
    assert(fb[32*64+32]==0xff0000); memcpy(saved,fb,sizeof(fb));
    pix2d_cls(); pix3d_begin_scene(); triangle(300,2,0); triangle(100,1,0); pix3d_end_scene();
    assert(!memcmp(fb,saved,sizeof(fb)));
    puts("PASS: reversing overlapping geometry submission produces identical pixels");
    // Alpha is deferred across all world objects, behind the opaque depth test.
    pix2d_cls(); pix3d_begin_scene(); triangle(100,1,128); triangle(300,2,0); pix3d_end_scene();
    assert(fb[32*64+32]==0x7f007f); memcpy(saved,fb,sizeof(fb));
    pix2d_cls(); pix3d_begin_scene(); triangle(300,2,0); triangle(100,1,128); pix3d_end_scene();
    assert(!memcmp(fb,saved,sizeof(fb)));
    pix2d_cls(); pix3d_begin_scene(); triangle(300,1,128); triangle(100,2,0); pix3d_end_scene();
    assert(fb[32*64+32]==0x0000ff);
    puts("PASS: alpha composites after opaque geometry and respects foreground depth");
    // A transparent texture texel cannot hide geometry submitted later.
    uint32_t transparent[1]={0}; pix3d_set_texture(0,transparent,1,1);
    Pix3DVertex a=vertex(8,8,100,0), b=vertex(32,56,100,0), c=vertex(56,8,100,0);
    pix2d_cls(); pix3d_begin_scene(); pix3d_scene_triangle(a,b,c,0,0); triangle(300,2,0); pix3d_end_scene();
    assert(fb[32*64+32]==0x0000ff);
    puts("PASS: transparent texture holes preserve background visibility and depth");
    // Perspective test: at this pixel affine U is > .5, perspective U is < .5.
    uint32_t tex[2]={0xff0000,0x0000ff}; pix3d_set_texture(0,tex,2,1);
    a=vertex(8,8,100,0); b=vertex(32,56,400,0); c=vertex(56,8,400,0);
    b.u=1; c.u=1;
    pix2d_cls(); pix3d_begin_scene(); pix3d_scene_triangle(a,b,c,0,0); pix3d_end_scene();
    assert((fb[20*64+32]&0xff0000)!=0 && (fb[20*64+32]&255)==0);
    puts("PASS: UV sampling uses perspective-correct interpolation");
    // A face crossing z=50 is clipped, rather than discarded in its entirety.
    a=vertex(8,8,25,1); b=vertex(32,56,100,1); c=vertex(56,8,100,1);
    pix2d_cls(); pix3d_begin_scene(); pix3d_scene_triangle(a,b,c,-1,0); pix3d_end_scene();
    int pixels=0; for(int i=0;i<64*64;i++) pixels+=fb[i]!=0;
    assert(pixels>100);
    puts("PASS: near-plane crossing retains the visible part of a face");
    // Viewport clipping must protect the full UI border for textured faces too.
    for(int i=0;i<64*64;i++) fb[i]=0x123456;
    pix2d_set_clip(16,16,48,48); pix3d_begin_scene();
    pix3d_scene_triangle(a,b,c,0,0); pix3d_end_scene();
    for(int y=0;y<64;y++) for(int x=0;x<64;x++)
        if(x<16||x>=48||y<16||y>=48) assert(fb[y*64+x]==0x123456);
    // Axis vertices map exactly to the reference UV basis.
    Pix3DVertex axis0={0,0,100,0,0,0},axis1={2,0,100,0,0,0},axis2={0,2,100,0,0,0};
    Pix3DVertex v={1,1,100,0,0,0}; pix3d_map_texture(&v,axis0,axis1,axis2);
    assert(v.u==0.5 && v.v==0.5);
    puts("PASS: textured viewport clipping and cache texture-axis basis");
    puts("[Test Scene Rasterizer] ALL TESTS PASSED!");
    return 0;
}
