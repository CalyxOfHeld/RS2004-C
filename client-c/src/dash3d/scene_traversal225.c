/* Isolated revision-225 World3D.drawTile queue and painter order. */
#include "dash3d/scene_traversal225.h"
#include <stdlib.h>
#include <string.h>

static const int front[9]={19,55,38,155,255,110,137,205,76};
static const int allow[9]={160,192,80,96,0,144,80,48,160};
static const int back[9]={76,8,137,4,0,1,38,2,19};
static const int block16[9]={0,0,2,0,0,2,1,1,0};
static const int block32[9]={2,0,0,2,0,0,0,4,4};
static const int block64[9]={0,4,4,8,0,0,8,0,0};
static const int block128[9]={1,1,0,0,0,8,0,0,8};
static const int inset_x[4]={53,-53,-53,53},inset_z[4]={-53,-53,53,53};
static const int outset_x[4]={-45,45,45,-45},outset_z[4]={45,45,-45,-45};
static int32_t ji(uint32_t v){return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t add(int32_t a,int32_t b){return ji((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return ji((uint32_t)a-(uint32_t)b);}
static int32_t neg(int32_t a){return ji(0u-(uint32_t)a);}
static int32_t max(int32_t a,int32_t b){return a>b?a:b;}
static int32_t cell(const SceneTraversal225 *s,int l,int x,int z){
    if(l<0||l>=s->max_level||x<0||x>=s->grid_x||z<0||z>=s->grid_z)return -1;
    return s->grid[((size_t)l*s->grid_x+x)*s->grid_z+z];
}
static SceneGround225 *ground(SceneTraversal225 *s,int id){return id>=0&&(size_t)id<s->ground_count?&s->grounds[id]:NULL;}
static SceneGround225 *at(SceneTraversal225 *s,int l,int x,int z){return ground(s,cell(s,l,x,z));}
typedef struct {SceneTraversal225 *s;int head,tail;bool good;} Queue;
static void enqueue(Queue *q,SceneGround225 *g){
    if(!g||!q->good)return;
    SceneTraversal225 *s=q->s;int id=g->id;
    if(id<0||(size_t)id>=s->ground_count){q->good=false;return;}
    if(g->prev!=-1){
#ifdef SCENE225_FAULT_NO_MOVE
        return;
#endif
        int p=g->prev,n=g->next;
        if(p>=0)s->grounds[p].next=n;else q->head=n;
        if(n>=0)s->grounds[n].prev=p;else q->tail=p;
    }
    g->prev=q->tail;g->next=-2;
    if(q->tail>=0)s->grounds[q->tail].next=id;else q->head=id;
    q->tail=id;
}
static SceneGround225 *dequeue(Queue *q){
    if(q->head<0)return NULL;
    SceneGround225 *g=&q->s->grounds[q->head];int n=g->next;
    q->head=n>=0?n:-1;if(q->head>=0)q->s->grounds[q->head].prev=-2;else q->tail=-1;
    g->prev=g->next=-1;return g;
}
static bool emit_ground(SceneTraversal225 *s,SceneGround225 *g,int level,bool overlay){
    SceneCall225 c={0};c.kind=overlay?SCENE225_OVERLAY:SCENE225_UNDERLAY;
    c.ground_id=g->id;c.level=level;c.x=g->x;c.z=g->z;
    return s->emit(s->emit_user,&c);
}
static bool emit_model(SceneTraversal225 *s,const uint8_t digest[32],int yaw,int x,int y,int z,int bitset){
    static const uint8_t empty[32]={0};if(memcmp(digest,empty,32)==0)return false;
    SceneCall225 c={0};c.kind=SCENE225_MODEL;c.ground_id=-1;c.yaw=yaw;
    c.dx=sub(x,s->eye_x);c.dy=sub(y,s->eye_y);c.dz=sub(z,s->eye_z);c.bitset=bitset;
    memcpy(c.model,digest,32);return s->emit(s->emit_user,&c);
}
static bool location_model(SceneTraversal225 *s,const SceneLocation225 *p,uint8_t key[32],int32_t *max_y){
    static const uint8_t empty[32]={0};
    memcpy(key,p->model,32);*max_y=p->max_y;
    if(memcmp(key,empty,32))return true; /* Original direct-model precedence. */
    return s->resolve_entity && s->resolve_entity(s->entity_user,p,key,max_y) && memcmp(key,empty,32)!=0;
}
static bool decor(SceneTraversal225 *s,const SceneDecor225 *d,int mask,bool rear){
    if(!d->present)return true;
    if((d->type&mask)!=0)return emit_model(s,d->model,d->angle,d->x,d->y,d->z,d->bitset);
    if((d->type&0x300)==0)return true;
    int a=sub(d->x,s->eye_x),z=sub(d->z,s->eye_z),angle=d->angle;
    if(angle<0||angle>3)return false;
    int ax=(angle==1||angle==2)?neg(a):a,az=(angle==2||angle==3)?neg(z):z;
    if((d->type&0x100)!=0 && (rear?az>=ax:az<ax))
        if(!emit_model(s,d->model,angle*512+256,add(d->x,inset_x[angle]),d->y,add(d->z,inset_z[angle]),d->bitset))return false;
    if((d->type&0x200)!=0 && (rear?az<=ax:az>ax))
        if(!emit_model(s,d->model,(angle*512+1280)&2047,add(d->x,outset_x[angle]),d->y,add(d->z,outset_z[angle]),d->bitset))return false;
    return true;
}
static bool ground_objects(SceneTraversal225 *s,const SceneGroundObject225 *o,bool offset){
    if(!o->present||((o->offset!=0)!=offset))return true;
    int y=offset?sub(o->y,o->offset):o->y;
    static const uint8_t empty[32]={0};
    if(memcmp(o->bottom,empty,32)&&!emit_model(s,o->bottom,0,o->x,y,o->z,o->bitset))return false;
    if(memcmp(o->middle,empty,32)&&!emit_model(s,o->middle,0,o->x,y,o->z,o->bitset))return false;
    if(memcmp(o->top,empty,32)&&!emit_model(s,o->top,0,o->x,y,o->z,o->bitset))return false;
    return true;
}
static bool visible_ground(SceneTraversal225 *s,Queue *q,SceneGround225 *g){
    int x=g->x,z=g->z,l=g->level,ol=g->occlude_level;
    SceneGround225 *bridge=ground(s,g->bridge);
    if(bridge){
        if(bridge->underlay){if(!world3d_tile_occluded225(&s->occlusion,0,x,z)&&!emit_ground(s,bridge,0,false))return false;}
        else if(bridge->overlay && !world3d_tile_occluded225(&s->occlusion,0,x,z) && !emit_ground(s,bridge,0,true))return false;
        if(bridge->wall.present && !emit_model(s,bridge->wall.model_a,0,bridge->wall.x,bridge->wall.y,bridge->wall.z,bridge->wall.bitset))return false;
        for(int i=0;i<bridge->loc_count;i++){
            int id=bridge->locs[i];if(id<0||(size_t)id>=s->location_count)return false;
            SceneLocation225 *p=&s->locations[id];
            uint8_t key[32];int32_t max_y;
            if(!location_model(s,p,key,&max_y))return false;
            if(!emit_model(s,key,p->yaw,p->x,p->y,p->z,p->bitset))return false;
        }
    }
    bool drew=false;
    if(g->underlay){if(!world3d_tile_occluded225(&s->occlusion,ol,x,z)){drew=true;if(!emit_ground(s,g,ol,false))return false;}}
    else if(g->overlay&&!world3d_tile_occluded225(&s->occlusion,ol,x,z)){drew=true;if(!emit_ground(s,g,ol,true))return false;}
    int quadrant=0,front_mask=0;
    if(g->wall.present||g->decor.present){
        if(s->eye_tile_x==x)quadrant++;else if(s->eye_tile_x<x)quadrant+=2;
        if(s->eye_tile_z==z)quadrant+=3;else if(s->eye_tile_z>z)quadrant+=6;
        front_mask=front[quadrant];g->back_wall_types=back[quadrant];
    }
    SceneWall225 *w=&g->wall;
    if(w->present){
        if(!(w->type_a&allow[quadrant]))g->check_loc_spans=0;
        else{
            int span=w->type_a==16?3:w->type_a==32?6:w->type_a==64?12:9;
            int block=w->type_a==16?block16[quadrant]:w->type_a==32?block32[quadrant]:w->type_a==64?block64[quadrant]:block128[quadrant];
            g->check_loc_spans=span;g->block_loc_spans=block;g->inverse_block_loc_spans=span-block;
        }
        if((w->type_a&front_mask)&&!world3d_wall_occluded225(&s->occlusion,ol,x,z,w->type_a))
            if(!emit_model(s,w->model_a,0,w->x,w->y,w->z,w->bitset))return false;
        if((w->type_b&front_mask)&&!world3d_wall_occluded225(&s->occlusion,ol,x,z,w->type_b))
            if(!emit_model(s,w->model_b,0,w->x,w->y,w->z,w->bitset))return false;
    }
    if(g->decor.present && !world3d_object_occluded225(&s->occlusion,ol,x,z,g->decor.max_y))
        if(!decor(s,&g->decor,front_mask,false))return false;
    if(drew){
        SceneGroundDecor225 *d=&g->ground_decor;
        if(d->present&&!emit_model(s,d->model,0,d->x,d->y,d->z,d->bitset))return false;
        if(!ground_objects(s,&g->ground_obj,false))return false;
    }
    int spans=g->loc_spans;
    if(spans){
        SceneGround225 *n;
        if(x<s->eye_tile_x&&(spans&4)&&(n=at(s,l,x+1,z))&&n->update)enqueue(q,n);
        if(z<s->eye_tile_z&&(spans&2)&&(n=at(s,l,x,z+1))&&n->update)enqueue(q,n);
        if(x>s->eye_tile_x&&(spans&1)&&(n=at(s,l,x-1,z))&&n->update)enqueue(q,n);
        if(z>s->eye_tile_z&&(spans&8)&&(n=at(s,l,x,z-1))&&n->update)enqueue(q,n);
    }
    return q->good;
}
static bool location_phase(SceneTraversal225 *s,Queue *q,SceneGround225 *g){
    int l=g->level,x=g->x,z=g->z,ol=g->occlude_level;
    if(g->check_loc_spans){
        bool ready=true;
        for(int i=0;i<g->loc_count;i++){
            int id=g->locs[i];if(id<0||(size_t)id>=s->location_count)return false;
            if(s->locations[id].cycle!=s->cycle && (g->loc_span[i]&g->check_loc_spans)==g->block_loc_spans){ready=false;break;}
        }
        if(ready){SceneWall225 *w=&g->wall;
            if(!w->present)return false;
            if(!world3d_wall_occluded225(&s->occlusion,ol,x,z,w->type_a))
                if(!emit_model(s,w->model_a,0,w->x,w->y,w->z,w->bitset))return false;
            g->check_loc_spans=0;
        }
    }
    if(!g->contains_locs)return true;
    int buffer[100],count=0;g->contains_locs=false;
    for(int i=0;i<g->loc_count;i++){
        int id=g->locs[i];if(id<0||(size_t)id>=s->location_count)return false;
        SceneLocation225 *p=&s->locations[id];if(p->cycle==s->cycle)continue;
        bool defer=false;
        for(int xx=p->min_x;xx<=p->max_x&&!defer;xx++)for(int zz=p->min_z;zz<=p->max_z;zz++){
            SceneGround225 *t=at(s,l,xx,zz);if(!t)return false;
            if(t->visible){defer=true;break;}
            if(t->check_loc_spans){
                int mask=(xx>p->min_x?1:0)+(xx<p->max_x?4:0)+(zz>p->min_z?8:0)+(zz<p->max_z?2:0);
                if((mask&t->check_loc_spans)==g->inverse_block_loc_spans){defer=true;break;}
            }
        }
        if(defer){g->contains_locs=true;continue;}
        if(count>=100)return false;
        buffer[count++]=id;
        int dx=max(sub(s->eye_tile_x,p->min_x),sub(p->max_x,s->eye_tile_x));
        int dz=max(sub(s->eye_tile_z,p->min_z),sub(p->max_z,s->eye_tile_z));
        p->distance=add(dx,dz);
    }
    while(count){
        int greatest=-50,index=-1;
        for(int i=0;i<count;i++){SceneLocation225 *p=&s->locations[buffer[i]];
#ifdef SCENE225_FAULT_TIE_LAST
            if(p->distance>=greatest&&p->cycle!=s->cycle){greatest=p->distance;index=i;}
#else
            if(p->distance>greatest&&p->cycle!=s->cycle){greatest=p->distance;index=i;}
#endif
        }
        if(index<0)break;
        SceneLocation225 *p=&s->locations[buffer[index]];p->cycle=s->cycle;
        uint8_t key[32];int32_t max_y;
        if(!location_model(s,p,key,&max_y))return false;
        if(!world3d_loc_occluded225(&s->occlusion,ol,p->min_x,p->max_x,p->min_z,p->max_z,max_y))
            if(!emit_model(s,key,p->yaw,p->x,p->y,p->z,p->bitset))return false;
        for(int xx=p->min_x;xx<=p->max_x;xx++)for(int zz=p->min_z;zz<=p->max_z;zz++){
            SceneGround225 *t=at(s,l,xx,zz);if(!t)return false;
            if(t->check_loc_spans)enqueue(q,t);
            else if((xx!=x||zz!=z)&&t->update)enqueue(q,t);
        }
        if(!q->good)return false;
    }
    return true;
}
static bool final_phase(SceneTraversal225 *s,Queue *q,SceneGround225 *g){
    int l=g->level,x=g->x,z=g->z,ol=g->occlude_level;
    g->update=false;s->tiles_remaining--;
    if(!ground_objects(s,&g->ground_obj,true))return false;
    if(g->back_wall_types){
        if(g->decor.present&&!world3d_object_occluded225(&s->occlusion,ol,x,z,g->decor.max_y))
            if(!decor(s,&g->decor,g->back_wall_types,true))return false;
        SceneWall225 *w=&g->wall;
        if(w->present){
            if((w->type_b&g->back_wall_types)&&!world3d_wall_occluded225(&s->occlusion,ol,x,z,w->type_b))
                if(!emit_model(s,w->model_b,0,w->x,w->y,w->z,w->bitset))return false;
            if((w->type_a&g->back_wall_types)&&!world3d_wall_occluded225(&s->occlusion,ol,x,z,w->type_a))
                if(!emit_model(s,w->model_a,0,w->x,w->y,w->z,w->bitset))return false;
        }
    }
    SceneGround225 *n;
    if(l<s->max_level-1&&(n=at(s,l+1,x,z))&&n->update)enqueue(q,n);
    if(x<s->eye_tile_x&&(n=at(s,l,x+1,z))&&n->update)enqueue(q,n);
    if(z<s->eye_tile_z&&(n=at(s,l,x,z+1))&&n->update)enqueue(q,n);
    if(x>s->eye_tile_x&&(n=at(s,l,x-1,z))&&n->update)enqueue(q,n);
    if(z>s->eye_tile_z&&(n=at(s,l,x,z-1))&&n->update)enqueue(q,n);
    return q->good;
}
bool world3d_traverse225(SceneTraversal225 *s,int32_t root_id,bool first_pass){
    if(!s||!s->grounds||!s->grid||!s->locations||!s->emit||root_id<0||(size_t)root_id>=s->ground_count)return false;
    Queue q={s,-1,-1,true};enqueue(&q,&s->grounds[root_id]);
    SceneGround225 *g;
    while((g=dequeue(&q))){
        if(!g->update)continue;
        int x=g->x,z=g->z,l=g->level;SceneGround225 *n;
        if(g->visible){
            if(first_pass){
                if(l>0&&(n=at(s,l-1,x,z))&&n->update)continue;
                if(x<=s->eye_tile_x&&x>s->min_x&&(n=at(s,l,x-1,z))&&n->update&&(n->visible||!(g->loc_spans&1)))continue;
                if(x>=s->eye_tile_x&&x<s->max_x-1&&(n=at(s,l,x+1,z))&&n->update&&(n->visible||!(g->loc_spans&4)))continue;
                if(z<=s->eye_tile_z&&z>s->min_z&&(n=at(s,l,x,z-1))&&n->update&&(n->visible||!(g->loc_spans&8)))continue;
                if(z>=s->eye_tile_z&&z<s->max_z-1&&(n=at(s,l,x,z+1))&&n->update&&(n->visible||!(g->loc_spans&2)))continue;
            }else first_pass=true;
            g->visible=false;
            if(!visible_ground(s,&q,g))return false;
        }
        if(!location_phase(s,&q,g))return false;
        if(g->contains_locs)continue;
        if(!g->update||g->check_loc_spans)continue;
        if(x<=s->eye_tile_x&&x>s->min_x&&(n=at(s,l,x-1,z))&&n->update)continue;
        if(x>=s->eye_tile_x&&x<s->max_x-1&&(n=at(s,l,x+1,z))&&n->update)continue;
        if(z<=s->eye_tile_z&&z>s->min_z&&(n=at(s,l,x,z-1))&&n->update)continue;
        if(z>=s->eye_tile_z&&z<s->max_z-1&&(n=at(s,l,x,z+1))&&n->update)continue;
        if(!final_phase(s,&q,g))return false;
    }
    return q.good;
}

static bool visit(SceneTraversal225 *s,int level,int x,int z,bool first_pass,int *first_root){
    SceneGround225 *g=at(s,level,x,z);
    if(!g||!g->visible)return true;
    if(*first_root>=0){
        if(g->id!=*first_root)return false;
        *first_root=-1;
    }
    return world3d_traverse225(s,g->id,first_pass);
}

bool world3d_traverse_frame225(SceneTraversal225 *s,int32_t expected_root_id){
    if(!s||s->min_level<0||s->max_level<=s->min_level||
       s->min_x<0||s->min_z<0||s->max_x>s->grid_x||s->max_z>s->grid_z||
       s->eye_tile_x<0||s->eye_tile_z<0)return false;
    int first_root=expected_root_id;
    for(int pass=0;pass<2;pass++){
        for(int l=s->min_level;l<s->max_level;l++){
            for(int dx=-25;dx<=0;dx++){
                int x0=s->eye_tile_x+dx,x1=s->eye_tile_x-dx;
                if(x0<s->min_x&&x1>=s->max_x)continue;
                for(int dz=-25;dz<=0;dz++){
                    int z0=s->eye_tile_z+dz,z1=s->eye_tile_z-dz;
                    if(x0>=s->min_x){
                        if(z0>=s->min_z&&!visit(s,l,x0,z0,pass==0,&first_root))return false;
                        if(z1<s->max_z&&!visit(s,l,x0,z1,pass==0,&first_root))return false;
                    }
                    if(x1<s->max_x){
                        if(z0>=s->min_z&&!visit(s,l,x1,z0,pass==0,&first_root))return false;
                        if(z1<s->max_z&&!visit(s,l,x1,z1,pass==0,&first_root))return false;
                    }
                    if(s->tiles_remaining==0){s->taking_input=false;return first_root==-1;}
                }
            }
        }
    }
    return first_root==-1;
}
