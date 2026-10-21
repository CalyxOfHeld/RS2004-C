#include "dash3d/location_anim.h"
#include "dash3d/world.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int frame_delay(const SeqType *seq,int frame) {
    return seq->delay[frame]==0 ? seq_get_delay(seq,frame):seq->delay[frame];
}

static WorldMapLocation *find(World3D *scene,int level,int x,int z,int layer) {
    for(size_t i=0;i<scene->map_location_count;i++) {
        WorldMapLocation *loc=&scene->map_locations[i];
        if(loc->level==level && loc->x==x && loc->z==z && world_location_layer(loc->shape)==layer) return loc;
    }
    return NULL;
}

bool location_anim_add(World3D *scene,int id,int level,int x,int z,int layer,int sequence,bool random_start) {
    if(!scene || !scene->sequences || sequence<0 || sequence>=scene->sequences->count) return false;
    const SeqType *seq=&scene->sequences->instances[sequence];
    if(seq->frame_count<=0 || !seq->frames || !seq->delay) return false;
    if(scene->location_animation_count==scene->location_animation_capacity) {
        size_t capacity=scene->location_animation_capacity ? scene->location_animation_capacity*2:128;
        LocationAnimation *items=realloc(scene->location_animations,capacity*sizeof(*items));
        if(!items) return false;
        scene->location_animations=items;scene->location_animation_capacity=capacity;
    }
    LocationAnimation a={id,level,x,z,layer,sequence,-1,0,random_start};
    if(random_start && seq->replayoff!=-1) {
        a.frame=(int)((double)rand()/((double)RAND_MAX+1)*seq->frame_count);
        a.cycle=(int)((double)rand()/((double)RAND_MAX+1)*frame_delay(seq,a.frame));
    }
    scene->location_animations[scene->location_animation_count++]=a;
    if(scene->trace_location_animations) printf("[LOCANIM] start id=%d tile=(%d,%d,%d) layer=%d seq=%d random=%d\n",id,level,x,z,layer,sequence,random_start);
    return true;
}

bool location_anim_packet(World3D *scene,int level,int x,int z,int layer,int sequence) {
    WorldMapLocation *loc=find(scene,level,x,z,layer);
    return !loc || location_anim_add(scene,loc->id,level,x,z,layer,sequence,false);
}

bool location_anim_advance(LocationAnimation *a,const SeqType *seq,int elapsed,bool *changed) {
    *changed=false;
    a->cycle+=elapsed;
    if(a->frame==-1) {a->frame=0;*changed=true;}
    while(a->cycle>frame_delay(seq,a->frame)) {
        a->cycle-=frame_delay(seq,a->frame)+1;
        a->frame++;*changed=true;
        if(a->frame>=seq->frame_count) {
            a->frame-=seq->replayoff;
            if(a->frame<0 || a->frame>=seq->frame_count) {*changed=false;return false;}
        }
    }
    return true;
}

void location_anim_tick(World3D *scene,LocConfig *locations,int elapsed) {
    if(!scene || !scene->sequences || elapsed<0) return;
    for(size_t i=0;i<scene->location_animation_count;) {
        LocationAnimation *a=&scene->location_animations[i];
        const SeqType *seq=&scene->sequences->instances[a->sequence];
        bool changed=false,active=location_anim_advance(a,seq,elapsed,&changed);
        WorldMapLocation *current=changed ? find(scene,a->level,a->x,a->z,a->layer):NULL;
        if(changed && (!current || current->id!=a->id)) active=false;
        if(active && changed) {
            LocType type;
            if(loc_get(locations,a->id,&type)) {
                GroundTile *tile=&scene->tiles[a->level][a->x][a->z];
                int part=0;
                for(int j=0;j<tile->loc_count;j++) {
                    WorldLocation *loc=&tile->locs[j];
                    if(((loc->bitset>>14)&32767)!=a->id || world_location_layer(loc->shape)!=a->layer) continue;
                    int shape=current->shape==11 ? 10:a->layer==1 ? 4:current->shape;
                    int rotation=a->layer==1 ? 0:current->rotation;
                    if(current->shape==2) rotation=part==0 ? rotation+4:(rotation+1)&3;
                    Model *model=loc_get_model_frame(&type,shape,rotation,seq->frames[a->frame],NULL);
                    loc_skew_model(model,&type,scene->heightmap[a->level][a->x][a->z],scene->heightmap[a->level][a->x+1][a->z],
                        scene->heightmap[a->level][a->x+1][a->z+1],scene->heightmap[a->level][a->x][a->z+1]);
                    if(model) {
                        if(loc->owns_model) model_free_unshared(loc->model);
                        loc->model=model;loc->owns_model=true;part++;
                        if(a->layer==1) loc->yaw=current->rotation*512;
                    }
                }
                if(scene->trace_location_animations && !a->random_start) printf("[LOCANIM] frame id=%d seq=%d frame=%d cycle=%d parts=%d\n",a->id,a->sequence,a->frame,a->cycle,part);
                loc_free_entry(&type);
            }
        }
        if(active) i++;
        else {
            memmove(a,a+1,(scene->location_animation_count-i-1)*sizeof(*a));scene->location_animation_count--;
        }
    }
}
