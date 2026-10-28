#include "dash3d/world3d.h"
#include <stdlib.h>

/* Tiles whose item list is non-empty, so the per-frame scene pass need not
 * scan every tile (ground_items_stocked). Updated wherever a list goes from
 * empty to non-empty or back; the caller filters stale or foreign pointers. */
static GroundTile **stocked;
static size_t stocked_count,stocked_capacity;
static bool stocked_lost;
static void stock_note(GroundTile *tile,bool had) {
    if(!had && tile->items) {
        if(stocked_count==stocked_capacity) {
            size_t capacity=stocked_capacity?stocked_capacity*2:256;
            GroundTile **grown=realloc(stocked,capacity*sizeof(*grown));
            if(!grown) {stocked_lost=true;return;}
            stocked=grown;stocked_capacity=capacity;
        }
        stocked[stocked_count++]=tile;
    } else if(had && !tile->items) {
        for(size_t i=0;i<stocked_count;i++) if(stocked[i]==tile) {stocked[i]=stocked[--stocked_count];break;}
    }
}
bool ground_items_stocked(GroundTile *const **tiles,size_t *count) {
    *tiles=stocked;*count=stocked_count;return !stocked_lost;
}

/* client.readZonePacket/sortObjStacks and ObjType.getInterfaceModel (rev 225).
 * Lists retain duplicate IDs; DEL removes the first and COUNT matches old count.
 * Each entry owns a fresh heap model, never a shared icon or arena view. */
void ground_items_clear(GroundTile *tile) {
    bool had=tile->items!=NULL;
    while(tile->items) {
        GroundItem *item=tile->items;tile->items=item->next;
        model_free_unshared(item->model);free(item);
    }
    stock_note(tile,had);
}
static Model *stack_model(ObjConfig *objects,ObjType *type,int count) {
    int variant=-1;
    if(count>1) for(int i=0;i<10;i++)
        if(type->countco[i]!=0 && count>=type->countco[i]) variant=type->countobj[i];
    ObjType other;
    if(variant>=0) {if(!obj_get(objects,variant,&other)) return NULL;type=&other;}
    Model *model=model_load(type->model,NULL);
    if(model) {
        for(int i=0;i<type->recol_count;i++) model_recolor(model,type->recol_s[i],type->recol_d[i]);
        model_calculate_normals(model,64,768,-50,-10,-50);
    }
    if(variant>=0) obj_free_entry(&other);
    return model;
}
static void sort_stack(GroundTile *tile) {
    GroundItem **best=&tile->items;
    for(GroundItem **p=&tile->items;*p;p=&(*p)->next)
        if((*p)->value>(*best)->value) best=p;
    if(*best && best!=&tile->items) {
        GroundItem *top=*best;*best=top->next;top->next=tile->items;tile->items=top;
    }
}
static bool item_event(GroundTile *tile,ObjConfig *objects,int opcode,int id,int count,int new_count) {
    GroundItem **p=&tile->items;
    if(opcode==49 || opcode==151) {
        while(*p && ((*p)->id!=(id&32767) || (opcode==151 && (*p)->count!=count))) p=&(*p)->next;
        if(!*p) return true;
        if(opcode==49) {
            GroundItem *old=*p;*p=old->next;model_free_unshared(old->model);free(old);
            if(tile->items) sort_stack(tile);
            return true;
        }
        id&=32767;count=new_count;
    } else while(*p) p=&(*p)->next;
    ObjType type;if(!obj_get(objects,id,&type)) return false;
    Model *model=stack_model(objects,&type,count);
    /* Java int multiplication wraps; avoid signed-overflow UB in C. */
    uint32_t raw=(uint32_t)type.cost*(type.stackable?(uint32_t)count+1u:1u);
    int32_t value=raw<=INT32_MAX?(int32_t)raw:(int32_t)((int64_t)raw-4294967296LL);
    obj_free_entry(&type);
    if(!model) return false;
    if(opcode==151) model_free_unshared((*p)->model);
    else { *p=calloc(1,sizeof(**p));if(!*p) {model_free_unshared(model);return false;} }
    (*p)->id=id;(*p)->count=count;(*p)->value=value;(*p)->model=model;
    sort_stack(tile);return true;
}
bool ground_items_event(GroundTile *tile,ObjConfig *objects,int opcode,int id,int count,int new_count) {
    bool had=tile->items!=NULL;
    bool ok=item_event(tile,objects,opcode,id,count,new_count);
    stock_note(tile,had);return ok;
}
int ground_items_models(GroundTile *tile,GroundItem *out[3]) {
    int count=0;
    for(GroundItem *p=tile->items;p && count<3;p=p->next) {
        bool duplicate=false;for(int i=0;i<count;i++) if(out[i]->id==p->id) duplicate=true;
        if(!duplicate) out[count++]=p;
    }
    return count;
}
GroundItem **ground_items_detach(World3D *scene) {
    GroundItem **saved=calloc(WORLD3D_MAX_LEVEL*WORLD3D_MAX_X*WORLD3D_MAX_Z,sizeof(*saved));
    if(!saved) return NULL;
    for(int l=0;l<scene->max_level;l++) for(int x=0;x<scene->max_tile_x;x++) for(int z=0;z<scene->max_tile_z;z++) {
        GroundTile *tile=&scene->tiles[l][x][z];bool had=tile->items!=NULL;
        saved[(l*WORLD3D_MAX_X+x)*WORLD3D_MAX_Z+z]=tile->items;
        tile->items=NULL;stock_note(tile,had);
    }
    return saved;
}
void ground_items_restore(World3D *scene,GroundItem **items,int dx,int dz) {
    if(!items) return;
    for(int l=0;l<scene->max_level;l++) for(int x=0;x<scene->max_tile_x;x++) for(int z=0;z<scene->max_tile_z;z++) {
        GroundItem *item=items[(l*WORLD3D_MAX_X+x)*WORLD3D_MAX_Z+z];
        if(!item) continue;
        if(x-dx>=0 && z-dz>=0 && x-dx<scene->max_tile_x && z-dz<scene->max_tile_z) {
            GroundTile *tile=&scene->tiles[l][x-dx][z-dz];bool had=tile->items!=NULL;
            tile->items=item;stock_note(tile,had);
        }
        else {GroundTile discarded={.items=item};ground_items_clear(&discarded);}
    }
    free(items);
}
