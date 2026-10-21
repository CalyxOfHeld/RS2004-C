#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/zone_sync.h"
#include "core/jagfile.h"
#include "gfx/pix3d.h"
static WorldMapLocation *find(World3D *scene,int level,int x,int z) {
    for(size_t i=0;i<scene->map_location_count;i++) {
        WorldMapLocation *loc=&scene->map_locations[i];if(loc->level==level && loc->x==x && loc->z==z) return loc;
    }return NULL;
}
int main(void) {
    Arena arena=arena_create(64*1024*1024);pix3d_init();pix3d_init_colour_table(0.8);
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);
    Jagfile *models=jagfile_load_file("../engine/data/pack/client/models",&arena);assert(config && models);
    model_init_repo(models,&arena);LocConfig locs;loc_unpack(&locs,config);
    int door=-1;
    for(int i=0;i<locs.count;i++) {LocType type;assert(loc_get(&locs,i,&type));
        bool wall=false;for(int j=0;j<type.model_count;j++) if(type.shapes[j]==0) wall=true;
        if(wall && type.blockwalk && !strcmp(type.name,"Door")) door=i;
        loc_free_entry(&type);if(door>=0) break;
    }assert(door>=0);printf("[CACHE] Door fixture id=%d\n",door);
    World *world=calloc(1,sizeof(World));World3D *scene=calloc(1,sizeof(World3D));assert(world && scene);
    world_init(world,104,104);world3d_init(scene,104,104,4);
    CollisionMap collision[4];for(int i=0;i<4;i++) collision_map_init(&collision[i],104,104);
    assert(world_change_location(world,scene,&locs,collision,0,50,50,door,0,0));
    assert(find(scene,0,50,50) && scene->tiles[0][50][50].loc_count==1 && scene->tiles[0][50][50].locs[0].owns_model);
    ZoneSync zone={0};
    uint8_t changed[]={48,48,59,0x22,1,(uint8_t)(door>>8),(uint8_t)door};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,changed,sizeof(changed))==1);
    assert(find(scene,0,50,50)->rotation==1 && zone.change_count==1 && zone.dirty);
    assert((collision[0].flags[50][50]&2) && !(collision[0].flags[50][50]&128));
    uint8_t malformed[]={48,48,59,0x22,2,(uint8_t)(door>>8),(uint8_t)door,76,0x22};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,malformed,sizeof(malformed))==-1);
    assert(find(scene,0,50,50)->rotation==1 && zone.change_count==1);
    uint8_t deleted[]={0x22,1};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,76,deleted,sizeof(deleted))==1);
    assert(!find(scene,0,50,50) && scene->tiles[0][50][50].loc_count==0 && collision[0].flags[50][50]==0);
    uint8_t full[]={48,48};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,135,full,sizeof(full))==1);
    assert(find(scene,0,50,50)->rotation==0 && scene->tiles[0][50][50].loc_count==1 && zone.change_count==0);
    for(int i=0;i<200;i++) {
        changed[4]=i&3;assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,changed,sizeof(changed))==1);
        assert(scene->tiles[0][50][50].loc_count==1 && scene->map_location_count==1);
    }
    world->tile_flags[1][60][60]=2;
    assert(world_change_location(world,scene,&locs,collision,1,60,60,door,0,0));
    assert(find(scene,1,60,60) && (collision[0].flags[60][60]&128) && collision[1].flags[60][60]==0);
    LocType type;assert(loc_get(&locs,door,&type));assert(!loc_get_model(&type,99,0,NULL));loc_free_entry(&type);
    puts("PASS: production enclosed/add/delete/full-zone location updates and original-state restoration using actual cache Door models");
    puts("PASS: malformed enclosed packet leaves scene unchanged; 200 replacements retain one owned model; LinkBelow changes collision plane only; missing model shape rejected");
    zone_sync_dispose(&zone);world3d_dispose(scene);free(scene);free(world);loc_free(&locs);arena_free(&arena);return 0;
}
