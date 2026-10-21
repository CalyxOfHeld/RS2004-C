#include <assert.h>
#include <stdio.h>
#include "dash3d/collision_map.h"
#include "dash3d/pathfinder.h"
static unsigned read_int(FILE *f) {
    unsigned n=0;for(int i=0;i<4;i++){int b=fgetc(f);assert(b!=EOF);n=(n<<8)|(unsigned)b;}return n;
}
static int read_bool(FILE *f) {int b=fgetc(f);assert(b==0 || b==1);return b;}
int main(void) {
    FILE *f=fopen("tests/fixtures/collision-reference.bin","rb");assert(f);
    CollisionMap map;collision_map_init(&map,104,104);
    for(int shape=0;shape<4;shape++) for(int r=0;r<4;r++) for(int range=0;range<2;range++) {
        collision_map_reset(&map);collision_map_add_wall(&map,50,50,shape,r,range!=0);
        for(int x=49;x<=51;x++) for(int z=49;z<=51;z++) assert((unsigned)map.flags[x][z]==read_int(f));
        collision_map_del_wall(&map,50,50,shape,r,range!=0);
        for(int x=49;x<=51;x++) for(int z=49;z<=51;z++) assert((unsigned)map.flags[x][z]==read_int(f));
    }
    int flags[]={0,1,2,4,8,16,32,64,128,0x100,0x200000,0xffffff};
    for(int flag=0;flag<12;flag++) for(int shape=0;shape<10;shape++) for(int r=0;r<4;r++) for(int x=48;x<=52;x++) for(int z=48;z<=52;z++) {
        collision_map_reset(&map);map.flags[x][z]=flags[flag];
        assert(collision_map_test_wall(&map,x,z,50,50,shape,r)==read_bool(f));
        assert(collision_map_test_decor(&map,x,z,50,50,shape,r)==read_bool(f));
    }
    for(int flag=0;flag<12;flag++) for(int force=0;force<16;force++) for(int x=48;x<=54;x++) for(int z=48;z<=54;z++) {
        collision_map_reset(&map);map.flags[x][z]=flags[flag];
        assert(collision_map_test_loc(&map,x,z,50,50,2,3,force)==read_bool(f));
    }
    for(int r=0;r<4;r++) for(int range=0;range<2;range++) {
        collision_map_reset(&map);collision_map_add_loc(&map,50,50,(r&1)?3:2,(r&1)?2:3,range!=0);
        for(int x=49;x<=54;x++) for(int z=49;z<=54;z++) assert((unsigned)map.flags[x][z]==read_int(f));
    }
    assert(fgetc(f)==EOF);fclose(f);
    Pathfinder pf;pathfinder_init(&pf);PathResult path;
    collision_map_reset(&map);collision_map_add_loc(&map,50,50,2,3,true);
    PathTarget target={.x=50,.z=50,.width=2,.length=3,.shape=-1};
    assert(pathfinder_find_target(&pf,&map,45,51,&target,false,&path));
    assert(path.steps_x[path.step_count-1]==49 && path.steps_z[path.step_count-1]>=50 && path.steps_z[path.step_count-1]<=52);
    assert(pathfinder_find_target(&pf,&map,49,51,&target,false,&path) && path.step_count==0);
    puts("PASS: 33,408 wall/decor/rectangular reach decisions and directional wall/rotated footprint flags match original Java CollisionMap output");
    puts("PASS: production pathfinder approaches blocked footprint and recognizes already-reached adjacency");
    return 0;
}
