#include "dash3d/location_merge.h"
#include "dash3d/camera.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool location_merge_packet(ZoneSync *zone,World *world,World3D *scene,LocConfig *locations,int level,const uint8_t *data) {
    if(!zone->entities) return true;
    int slot=(data[8]<<8)|data[9];
    PlayerEntity *player=slot==zone->local_pid ? &zone->entities->local_player:zone->entities->players[slot];
    if(!player) return true;
    int x=zone->base_x+((data[0]>>4)&7),z=zone->base_z+(data[0]&7);
    if(x<0 || z<0 || x>=scene->max_tile_x || z>=scene->max_tile_z) return true;
    int shape=data[1]>>2,rotation=data[1]&3,id=(data[2]<<8)|data[3],cycle=zone->entities->cycle;
    int start=cycle+((data[4]<<8)|data[5]),stop=cycle+((data[6]<<8)|data[7]);
    if(zone->merge_count+2>zone->merge_capacity) {
        size_t capacity=zone->merge_capacity ? zone->merge_capacity*2:32;
        ZoneLocationMerge *events=realloc(zone->merges,capacity*sizeof(*events));if(!events) return false;
        zone->merges=events;zone->merge_capacity=capacity;
    }
    LocType loc;if(!loc_get(locations,id,&loc)) return false;
    Model *model=loc_get_model(&loc,shape,rotation,NULL);
    loc_skew_model(model,&loc,scene->heightmap[level][x][z],scene->heightmap[level][x+1][z],scene->heightmap[level][x+1][z+1],scene->heightmap[level][x][z+1]);
    int width=rotation&1 ? loc.length:loc.width,length=rotation&1 ? loc.width:loc.length;
    player_entity_dispose(player);player->loc_model=model;
    player->loc_start_cycle=start;player->loc_stop_cycle=stop;
    player->loc_offset_x=x*128+width*64;player->loc_offset_z=z*128+length*64;
    player->loc_offset_y=camera_get_height(level,player->loc_offset_x,player->loc_offset_z,scene->heightmap,world->tile_flags);
    int east=data[10]<128 ? data[10]:data[10]-256,south=data[11]<128 ? data[11]:data[11]-256;
    int west=data[12]<128 ? data[12]:data[12]-256,north=data[13]<128 ? data[13]:data[13]-256;
    player->loc_min_tile_x=x+(east<west?east:west);player->loc_max_tile_x=x+(east>west?east:west);
    player->loc_min_tile_z=z+(south<north?south:north);player->loc_max_tile_z=z+(south>north?south:north);
    zone->merges[zone->merge_count++]=(ZoneLocationMerge){level,x,z,-1,shape,rotation,start};
    zone->merges[zone->merge_count++]=(ZoneLocationMerge){level,x,z,id,shape,rotation,stop};
    loc_free_entry(&loc);
    if(zone->trace) printf("[LOCMERGE] player=%d id=%d tile=(%d,%d,%d) start=%d stop=%d\n",slot,id,level,x,z,start,stop);
    return true;
}

bool location_merge_tick(ZoneSync *zone,World *world,World3D *scene,LocConfig *locations,CollisionMap *collision,int cycle) {
    for(size_t i=0;i<zone->merge_count;) {
        ZoneLocationMerge event=zone->merges[i];
        if(cycle<event.cycle) {i++;continue;}
        if(event.x>=1 && event.z>=1 && event.x<scene->max_tile_x-1 && event.z<scene->max_tile_z-1) {
            if(!world_change_location(world,scene,locations,collision,event.level,event.x,event.z,event.id,event.shape,event.rotation)) return false;
            zone->dirty=true;
        }
        if(zone->trace)printf("[LOCMERGE] scene id=%d tile=(%d,%d,%d) cycle=%d\n",event.id,event.level,event.x,event.z,cycle);
        memmove(zone->merges+i,zone->merges+i+1,(zone->merge_count-i-1)*sizeof(event));zone->merge_count--;
    }
    return true;
}
