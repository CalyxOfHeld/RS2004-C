#include "dash3d/scene_traversal225.h"
#include <stdio.h>
#include <string.h>

typedef struct {int calls;SceneCall225 first,last;} Capture;
static bool capture(void *user,const SceneCall225 *call){
    Capture *c=user;if(c->calls==0)c->first=*call;c->calls++;c->last=*call;return true;
}
int main(void){
    SceneGround225 grounds[2]={0};
    SceneLocation225 locations[1]={0};
    int32_t grid[1]={0};Capture out={0};
    grounds[0].id=0;grounds[0].level=0;grounds[0].x=0;grounds[0].z=0;
    grounds[0].visible=true;grounds[0].update=true;grounds[0].bridge=1;
    grounds[0].prev=grounds[0].next=-1;
    grounds[1].id=1;grounds[1].bridge=-1;grounds[1].prev=grounds[1].next=-1;
    grounds[1].wall.present=true;grounds[1].wall.x=140;grounds[1].wall.y=50;grounds[1].wall.z=260;
    grounds[1].wall.bitset=1234;grounds[1].wall.model_a[0]=0x45;
    SceneTraversal225 s={0};s.max_level=1;s.grid_x=s.grid_z=1;
    s.min_level=0;s.min_x=s.min_z=0;s.max_x=s.max_z=1;
    s.eye_x=100;s.eye_y=20;s.eye_z=200;
    s.eye_tile_x=s.eye_tile_z=0;s.tiles_remaining=1;s.taking_input=true;
    s.grounds=grounds;s.ground_count=2;s.locations=locations;s.location_count=1;
    s.grid=grid;s.emit=capture;s.emit_user=&out;
    if(!world3d_traverse_frame225(&s,0))return 1;
    if(out.calls!=1||out.last.kind!=SCENE225_MODEL||out.last.dx!=40||
       out.last.dy!=30||out.last.dz!=60||out.last.bitset!=1234||
       out.last.model[0]!=0x45||s.tiles_remaining!=0||s.taking_input)return 2;

    /* At equality the original front decoration test is strict (<), while
     * the deferred back test is inclusive (>=). The callback must occur once. */
    SceneGround225 edge={0};int32_t edge_grid[2]={-1,0};
    int32_t heights[6]={0},cycles[6]={0};Capture edge_out={0};
    edge.id=0;edge.level=0;edge.x=1;edge.z=0;edge.bridge=-1;
    edge.prev=edge.next=-1;edge.visible=edge.update=true;
    edge.decor.present=true;edge.decor.x=128;edge.decor.z=0;
    edge.decor.type=0x100;edge.decor.angle=0;edge.decor.model[0]=0x78;
    SceneTraversal225 e={0};e.max_level=1;e.grid_x=2;e.grid_z=1;
    e.min_level=0;e.min_x=e.min_z=0;e.max_x=2;e.max_z=1;
    e.eye_x=0;e.eye_y=0;e.eye_z=-128;e.eye_tile_x=e.eye_tile_z=0;
    e.tiles_remaining=1;e.grounds=&edge;e.ground_count=1;
    e.locations=locations;e.location_count=1;e.grid=edge_grid;
    e.emit=capture;e.emit_user=&edge_out;
    e.occlusion.heights=heights;e.occlusion.tile_cycles=cycles;
    e.occlusion.max_x=2;e.occlusion.max_z=1;e.occlusion.levels=1;e.occlusion.cycle=1;
    if(!world3d_traverse_frame225(&e,0))return 3;
    if(edge_out.calls!=1||edge_out.last.kind!=SCENE225_MODEL||
       edge_out.last.yaw!=256||edge_out.last.dx!=181||
       edge_out.last.dz!=75||edge_out.last.model[0]!=0x78)return 4;

    /* Equal location distances keep their first-buffer order (strict >). */
    SceneGround225 tie={0};SceneLocation225 tie_locs[2]={0};
    int32_t tie_grid[1]={0},tie_heights[4]={0},tie_cycles[4]={0};
    Capture tie_out={0};tie.id=0;tie.bridge=-1;tie.prev=tie.next=-1;
    tie.visible=tie.update=tie.contains_locs=true;
    tie.loc_count=2;tie.locs[0]=0;tie.locs[1]=1;
    tie_locs[0].model[0]=0x11;tie_locs[1].model[0]=0x22;
    SceneTraversal225 t={0};t.max_level=t.grid_x=t.grid_z=1;
    t.max_x=t.max_z=1;t.tiles_remaining=1;
    t.grounds=&tie;t.ground_count=1;t.locations=tie_locs;
    t.location_count=2;t.grid=tie_grid;t.emit=capture;t.emit_user=&tie_out;
    t.occlusion.heights=tie_heights;t.occlusion.tile_cycles=tie_cycles;
    t.occlusion.max_x=t.occlusion.max_z=t.occlusion.levels=1;
    t.occlusion.cycle=1;t.cycle=1;
    if(!world3d_traverse_frame225(&t,0))return 5;
    if(tie_out.calls!=2||tie_out.first.model[0]!=0x11||
       tie_out.last.model[0]!=0x22||t.tiles_remaining!=0)return 6;
    puts("scene traversal225 smoke: bridge, decoration equality, and location tie order match");
    return 0;
}
