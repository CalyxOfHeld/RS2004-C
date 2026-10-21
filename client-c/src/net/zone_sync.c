#include "net/zone_sync.h"
#include "dash3d/effects.h"
#include "dash3d/location_anim.h"
#include "dash3d/location_merge.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static const int zone_opcodes[10]={23,42,49,50,59,69,76,151,191,223};
static const int zone_lengths[10]={14,4,3,7,4,15,2,7,6,5};
static int event_index(int opcode) {for(int i=0;i<10;i++) if(zone_opcodes[i]==opcode) return i;return -1;}
void zone_sync_reset(ZoneSync *z) {z->base_x=z->base_z=0;z->has_base=z->dirty=false;z->change_count=z->merge_count=0;}
void zone_sync_dispose(ZoneSync *z) {free(z->changes);free(z->merges);memset(z,0,sizeof(*z));}
static bool remember(ZoneSync *z,World3D *scene,int level,int x,int y,int shape) {
    int layer=world_location_layer(shape);
    for(size_t i=0;i<z->change_count;i++) if(z->changes[i].level==level && z->changes[i].x==x && z->changes[i].z==y && z->changes[i].layer==layer) return true;
    if(z->change_count==z->change_capacity) {
        size_t cap=z->change_capacity ? z->change_capacity*2:128;
        ZoneLocationBaseline *p=realloc(z->changes,cap*sizeof(*p));if(!p) return false;
        z->changes=p;z->change_capacity=cap;
    }
    ZoneLocationBaseline old={level,x,y,layer,-1,shape,0};
    for(size_t i=0;i<scene->map_location_count;i++) {
        WorldMapLocation *loc=&scene->map_locations[i];
        if(loc->level==level && loc->x==x && loc->z==y && world_location_layer(loc->shape)==layer) {
            old.id=loc->id;old.shape=loc->shape;old.rotation=loc->rotation;break;
        }
    }
    z->changes[z->change_count++]=old;return true;
}
static void unsupported(ZoneSync *z,int index) {
    if(!(z->unsupported&(1u<<index)) && z->trace) fprintf(stderr,"[ZONE] Event %d framed but behavior remains unimplemented\n",zone_opcodes[index]);
    z->unsupported|=1u<<index;
}
static bool apply_event(ZoneSync *z,World *world,World3D *scene,LocConfig *locs,CollisionMap *collision,int level,int opcode,const uint8_t *data) {
    if(opcode==23) return location_merge_packet(z,world,scene,locs,level,data);
    if(opcode==42) {
        int x=z->base_x+((data[0]>>4)&7),y=z->base_z+(data[0]&7);
        if(x<0 || y<0 || x>=scene->max_tile_x || y>=scene->max_tile_z) return true;
        return location_anim_packet(scene,level,x,y,world_location_layer(data[1]>>2),(data[2]<<8)|data[3]);
    }
    if((opcode==69 || opcode==191) && z->effects)
        return effects_add_packet(z->effects,world,scene,level,z->base_x,z->base_z,opcode,data);
    if(opcode==223 || opcode==49 || opcode==151 || opcode==50) {
        int x=z->base_x+((data[0]>>4)&7),y=z->base_z+(data[0]&7);
        if(x<0 || y<0 || x>=scene->max_tile_x || y>=scene->max_tile_z) return true;
        if(opcode==50 && ((data[5]<<8)|data[6])==z->local_pid) return true;
        int id=(data[1]<<8)|data[2],count=opcode==49?0:(data[3]<<8)|data[4];
        int next=opcode==151?(data[5]<<8)|data[6]:0;
        if(!ground_items_event(&scene->tiles[level][x][y],z->objects,opcode,id,count,next)) return false;
        if(z->trace) printf("[GROUND] opcode=%d tile=(%d,%d,%d) id=%d count=%d new=%d\n",opcode,level,x,y,id,count,next);
        return true;
    }
    if(opcode!=59 && opcode!=76) {unsupported(z,event_index(opcode));return true;}
    int x=z->base_x+((data[0]>>4)&7),y=z->base_z+(data[0]&7),shape=data[1]>>2,rotation=data[1]&3;
    int id=opcode==59 ? (data[2]<<8)|data[3]:-1;
    if(x<=0 || y<=0 || x>=world->max_tile_x-1 || y>=world->max_tile_z-1) return true;
    if(!remember(z,scene,level,x,y,shape)) return false;
    if(!world_change_location(world,scene,locs,collision,level,x,y,id,shape,rotation)) return false;
    z->dirty=true;
    if(z->trace) printf("[ZONE] location=(%d,%d,%d) id=%d shape=%d rotation=%d\n",level,x,y,id,shape,rotation);
    return true;
}
static bool valid_item(ZoneSync *z,int op,const uint8_t *data) {
    if((op==69 || op==191) && z->effects && z->effects->config) {
        int id=op==69?(data[5]<<8)|data[6]:(data[1]<<8)|data[2];
        return id<z->effects->config->count;
    }
    if(op!=223 && op!=49 && op!=151 && op!=50) return true;
    int id=(data[1]<<8)|data[2];if(op==49 || op==151) id&=32767;
    return z->objects && id<z->objects->count;
}
int zone_sync_apply(ZoneSync *z,World *world,World3D *scene,LocConfig *locs,CollisionMap *collision,int level,int opcode,const uint8_t *data,size_t length) {
    int index=event_index(opcode);
    if(opcode!=7 && opcode!=135 && opcode!=162 && index<0) return 0;
    if(!z || !world || !scene || !locs || !data || level<0 || level>=4) return -1;
    // Framing/shape/type validation before mutation, including every enclosed event.
    size_t pos=opcode==162 ? 2:0;
    if(opcode==7 || opcode==135) {if(length!=2) return -1;}
    else if(opcode==162) {
        if(length<2) return -1;
        while(pos<length) {
            int op=data[pos++],i=event_index(op);if(i<0 || (size_t)zone_lengths[i]>length-pos) return -1;
            if((op==59 || op==76 || op==42 || op==23) && data[pos+1]>>2>22) return -1;
            if(op==42 && (!scene->sequences || ((data[pos+2]<<8)|data[pos+3])>=scene->sequences->count)) return -1;
            if((op==59 || op==23) && ((data[pos+2]<<8)|data[pos+3])>=locs->count) return -1;
            if(op==23 && ((data[pos+8]<<8)|data[pos+9])>=MAX_PLAYERS) return -1;
            if(!valid_item(z,op,data+pos)) return -1;
            pos+=zone_lengths[i];
        }
    } else {
        if(length!=(size_t)zone_lengths[index] || !z->has_base) return -1;
        if((opcode==59 || opcode==76 || opcode==42 || opcode==23) && data[1]>>2>22) return -1;
        if(opcode==42 && (!scene->sequences || ((data[2]<<8)|data[3])>=scene->sequences->count)) return -1;
        if((opcode==59 || opcode==23) && ((data[2]<<8)|data[3])>=locs->count) return -1;
        if(opcode==23 && ((data[8]<<8)|data[9])>=MAX_PLAYERS) return -1;
        if(!valid_item(z,opcode,data)) return -1;
    }
    if(opcode==7 || opcode==135 || opcode==162) {z->base_x=data[0];z->base_z=data[1];z->has_base=true;}
    if(opcode==135) {
        // Original read(135) restores each temporary location's previous state.
        for(size_t i=0;i<z->change_count;) {
            ZoneLocationBaseline old=z->changes[i];
            if(old.level!=level || old.x<z->base_x || old.x>=z->base_x+8 || old.z<z->base_z || old.z>=z->base_z+8) {i++;continue;}
            if(!world_change_location(world,scene,locs,collision,level,old.x,old.z,old.id,old.shape,old.rotation)) return -1;
            memmove(&z->changes[i],&z->changes[i+1],(z->change_count-i-1)*sizeof(old));z->change_count--;z->dirty=true;
        }
        for(int x=z->base_x;x<z->base_x+8 && x<scene->max_tile_x;x++)
            for(int y=z->base_z;y<z->base_z+8 && y<scene->max_tile_z;y++)
                ground_items_clear(&scene->tiles[level][x][y]);
        return 1;
    }
    if(opcode==162) {
        pos=2;
        while(pos<length) {int op=data[pos++],i=event_index(op);if(!apply_event(z,world,scene,locs,collision,level,op,data+pos)) return -1;pos+=zone_lengths[i];}
    } else if(index>=0 && !apply_event(z,world,scene,locs,collision,level,opcode,data)) return -1;
    return 1;
}
