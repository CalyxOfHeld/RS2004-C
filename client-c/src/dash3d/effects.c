#include "dash3d/effects.h"
#include "dash3d/camera.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void projectile_velocity(SceneEffect *p,int y,int z,int x,int cycle) {
    if(!p->mobile) {
        double dx=x-p->src_x,dz=z-p->src_z,dist=sqrt(dx*dx+dz*dz);
        // A coincident source/target produces NaN in Java. Keep malformed
        // zero-length shots out of C's undefined float-to-int conversions.
        if(dist==0) {p->complete=true;return;}
        p->x=p->src_x+dx*p->arc/dist;p->z=p->src_z+dz*p->arc/dist;p->y=p->src_y;
    }
    double time=p->last_cycle+1-cycle;
    if(time<=0) {p->complete=true;return;}
    p->vx=(x-p->x)/time;p->vz=(z-p->z)/time;p->velocity=sqrt(p->vx*p->vx+p->vz*p->vz);
    if(!p->mobile)p->vy=-p->velocity*tan(p->peak_pitch*0.02454369);
    p->ay=(y-p->y-p->vy*time)*2.0/(time*time);
}
void effect_advance(SceneEffect *p,const SpotAnimConfig *cfg,int delta) {
    if(p->projectile) {
        p->mobile=true;p->x+=p->vx*delta;p->z+=p->vz*delta;
        p->y+=p->vy*delta+p->ay*0.5*delta*delta;p->vy+=p->ay*delta;
        p->yaw=((int)(atan2(p->vx,p->vz)*325.949)+1024)&2047;
        p->pitch=((int)(atan2(p->vy,p->velocity)*325.949))&2047;
    }
    const SeqType *s=spotanim_seq(cfg,p->id);if(!s || s->frame_count<=0) {if(!p->projectile)p->complete=true;return;}
    p->frame_cycle+=delta;
    while(p->frame_cycle>seq_get_delay(s,p->frame)) {
        p->frame_cycle-=seq_get_delay(s,p->frame)+1;
        if(++p->frame>=s->frame_count) {p->frame=0;if(!p->projectile)p->complete=true;}
    }
}
void effects_clear(SceneEffects *f) {if(f)f->count=0;}
void effects_dispose(SceneEffects *f) {free(f->items);f->items=NULL;f->capacity=f->count=0;}
bool effects_add_packet(SceneEffects *f,World *w,World3D *scene,int level,int bx,int bz,int op,const uint8_t *d) {
    if(!f || !f->config) return true;
    SceneEffect p={0};p.projectile=op==69;p.level=level;
    int x=bx+((d[0]>>4)&7),z=bz+(d[0]&7);
    if(x<0 || z<0 || x>=104 || z>=104)return true;
    p.id=p.projectile?(d[5]<<8)|d[6]:(d[1]<<8)|d[2];
    if(p.id>=f->config->count)return false;
    p.src_x=x*128+64;p.src_z=z*128+64;
    if(p.projectile) {
        int tx=x+(int)(int8_t)d[1],tz=z+(int)(int8_t)d[2];
        if(tx<0 || tz<0 || tx>=104 || tz>=104)return true;
        p.target=(int16_t)((d[3]<<8)|d[4]);p.offset_y=d[8];p.peak_pitch=d[13];p.arc=d[14];
        p.start_cycle=f->cycle+((d[9]<<8)|d[10]);p.last_cycle=f->cycle+((d[11]<<8)|d[12]);
        p.src_y=camera_get_height(level,p.src_x,p.src_z,scene->heightmap,w->tile_flags)-d[7];
        projectile_velocity(&p,camera_get_height(level,tx*128+64,tz*128+64,scene->heightmap,w->tile_flags)-p.offset_y,tz*128+64,tx*128+64,p.start_cycle);
    } else {
        p.x=p.src_x;p.z=p.src_z;p.y=camera_get_height(level,p.src_x,p.src_z,scene->heightmap,w->tile_flags)-d[3];
        p.start_cycle=f->cycle+((d[4]<<8)|d[5]);
    }
    if(p.complete)return true;
    if(f->count==f->capacity) {
        size_t cap=f->capacity?f->capacity*2:32;
        // Native resource ceiling; fail explicitly instead of silently dropping.
        if(cap>4096)return false;
        SceneEffect *items=realloc(f->items,cap*sizeof(*items));if(!items)return false;f->items=items;f->capacity=cap;
    }
    f->items[f->count++]=p;
    if(f->trace)printf("[EFFECT] add opcode=%d id=%d start=%d last=%d target=%d\n",op,p.id,p.start_cycle,p.last_cycle,p.target);
    return true;
}
void effects_tick(SceneEffects *f,World *w,World3D *scene,EntitySyncContext *entities,int cycle) {
    f->cycle=cycle;
    for(size_t i=0;i<f->count;) {
        SceneEffect *p=&f->items[i];
        if(p->level!=entities->current_plane || p->complete || (p->projectile && cycle>p->last_cycle)) {
            memmove(p,p+1,(f->count-i-1)*sizeof(*p));f->count--;continue;
        }
        if(cycle>=p->start_cycle) {
            PathingEntity *target=NULL;
            if(p->projectile && p->target>0 && p->target<=MAX_NPCS && entities->npcs[p->target-1])target=&entities->npcs[p->target-1]->base;
            if(p->projectile && p->target<0) {
                int id=-p->target-1;
                if(id==entities->local_player_index)target=&entities->local_player.base;
                else if(id<MAX_PLAYERS && entities->players[id])target=&entities->players[id]->base;
            }
            if(target)projectile_velocity(p,camera_get_height(p->level,target->x,target->z,scene->heightmap,w->tile_flags)-p->offset_y,target->z,target->x,cycle);
            if(!p->complete)effect_advance(p,f->config,1);
        }
        i++;
    }
}
void effects_draw(SceneEffects *f,Arena *frame,int level,int ex,int ey,int ez,int pitch,int yaw) {
    for(size_t i=0;i<f->count;i++) {
        SceneEffect *p=&f->items[i];if(p->complete || p->level!=level || f->cycle<p->start_cycle)continue;
        // Invalid/extreme slopes can produce huge ballistic heights. Avoid
        // undefined C double-to-int conversion before the renderer culls them.
        if(!isfinite(p->x) || !isfinite(p->y) || !isfinite(p->z) ||
            fabs(p->x)>1000000 || fabs(p->y)>1000000 || fabs(p->z)>1000000)continue;
        size_t mark=frame->offset;
        Model *m=spotanim_model(f->config,p->id,p->frame,p->projectile?p->pitch:0,0,!p->projectile,frame);
        if(m) {
            model_draw(m,p->yaw,pitch,yaw,(int)p->x-ex,(int)p->y-ey,(int)p->z-ez);
            if(f->trace)printf("[EFFECT] draw id=%d cycle=%d xyz=(%d,%d,%d) frame=%d\n",p->id,f->cycle,(int)p->x,(int)p->y,(int)p->z,p->frame);
        }
        frame->offset=mark;
    }
}
void actor_spotanim_tick(PathingEntity *a,const SpotAnimConfig *c,int cycle) {
    if(a->spotanim_id<0 || cycle<a->spotanim_delay)return;
    const SeqType *s=spotanim_seq(c,a->spotanim_id);if(!s){a->spotanim_id=-1;return;}
    if(a->spotanim_frame<0)a->spotanim_frame=0;
    a->spotanim_cycle++;
    while(a->spotanim_frame<s->frame_count && a->spotanim_cycle>seq_get_delay(s,a->spotanim_frame)) {
        a->spotanim_cycle-=seq_get_delay(s,a->spotanim_frame);a->spotanim_frame++;
    }
    if(a->spotanim_frame>=s->frame_count)a->spotanim_id=-1;
}
Model *actor_spotanim_model(PathingEntity *a,SpotAnimConfig *c,Arena *frame) {
    return a->spotanim_frame<0?NULL:spotanim_model(c,a->spotanim_id,a->spotanim_frame,0,a->spotanim_height,false,frame);
}
Model *actor_with_spotanim(Model *body,PathingEntity *a,SpotAnimConfig *c,Arena *frame) {
    a->height=0;for(int i=0;i<body->num_points;i++)if(-body->point_y[i]>a->height)a->height=-body->point_y[i];
    Model *spot=actor_spotanim_model(a,c,frame);if(!spot)return body;
    size_t need=65536u+(size_t)(body->num_points+spot->num_points)*128u+(size_t)(body->num_faces+spot->num_faces)*128u;
    if(need>frame->capacity-frame->offset)return body;
    Model *parts[]={body,spot};Model *m=model_create_composite(parts,2,frame);if(!m)return body;
    // Original two-model constructor preserves each already lit face. The
    // general native composite builds geometry; retain the original lighting.
    m->face_colour_a=arena_alloc(frame,m->num_faces*sizeof(int32_t));
    m->face_colour_b=arena_alloc(frame,m->num_faces*sizeof(int32_t));
    m->face_colour_c=arena_alloc(frame,m->num_faces*sizeof(int32_t));
    int out=0;
    for(int n=0;n<2;n++) for(int i=0;i<parts[n]->num_faces;i++,out++) {
        m->face_colour_a[out]=parts[n]->face_colour_a[i];
        m->face_colour_b[out]=parts[n]->face_colour_b[i];
        m->face_colour_c[out]=parts[n]->face_colour_c[i];
    }
    return m;
}
