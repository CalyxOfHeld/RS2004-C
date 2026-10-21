#include "ui/component_packets.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

typedef struct { const uint8_t *data; size_t size,pos; bool error; } Reader;
static uint32_t read_be(Reader *r,size_t bytes) {
    if(r->pos>r->size || bytes>r->size-r->pos) {r->error=true;return 0;}
    uint32_t value=0; while(bytes--) value=(value<<8)|r->data[r->pos++]; return value;
}
static int read_quantity(Reader *r) {
    uint32_t value=read_be(r,1); if(value==255) value=read_be(r,4);
    if(value>INT_MAX) r->error=true;
    return (int)value;
}
static int inventory_packet(ComponentConfig *cfg,Arena *arena,int opcode,Reader *r) {
    int id=read_be(r,2); Component *c=component_get(cfg,id);
    if(r->error || !c || (c->type!=COMPONENT_TYPE_INV_GRID && c->type!=COMPONENT_TYPE_TEXT_LIST)) return -1;
    if(c->width<=0 || c->height<=0 || c->width>65536/c->height) return -1;
    int capacity=c->width*c->height;
    if(opcode==15) {
        if(r->pos!=r->size) return -1;
        if(c->inv_slot_obj_id) memset(c->inv_slot_obj_id,0,capacity*sizeof(int));
        if(c->inv_slot_obj_count) memset(c->inv_slot_obj_count,0,capacity*sizeof(int));
        return 1;
    }
    int *ids=calloc(capacity,sizeof(int)),*counts=calloc(capacity,sizeof(int));
    if(!ids||!counts) {free(ids);free(counts);return -1;}
    if(opcode==213) {
        if(c->inv_slot_obj_id) memcpy(ids,c->inv_slot_obj_id,capacity*sizeof(int));
        if(c->inv_slot_obj_count) memcpy(counts,c->inv_slot_obj_count,capacity*sizeof(int));
    }
    int total=opcode==98 ? (int)read_be(r,1) : 0;
    if(total>capacity) r->error=true;
    int index=0;
    while(!r->error && (opcode==98 ? index<total : r->pos<r->size)) {
        int slot=opcode==98 ? index++ : (int)read_be(r,1);
        int object=read_be(r,2), count=read_quantity(r);
        if(slot>=capacity) {r->error=true;break;}
        ids[slot]=object;counts[slot]=object ? count : 0;
    }
    if(r->error || r->pos!=r->size) {free(ids);free(counts);return -1;}
    if(!c->inv_slot_obj_id) c->inv_slot_obj_id=arena_alloc_zero(arena,capacity*sizeof(int));
    if(!c->inv_slot_obj_count) c->inv_slot_obj_count=arena_alloc_zero(arena,capacity*sizeof(int));
    if(!c->inv_slot_obj_id || !c->inv_slot_obj_count) {free(ids);free(counts);return -1;}
    memcpy(c->inv_slot_obj_id,ids,capacity*sizeof(int));
    memcpy(c->inv_slot_obj_count,counts,capacity*sizeof(int));
    free(ids);free(counts);return 1;
}
int component_apply_packet(ComponentConfig *cfg,VarpContext *varps,Arena *arena,
                           int opcode,const uint8_t *data,size_t length) {
    if(!cfg || !varps || !arena || (!data && length)) return -1;
    Reader r={data,length,0,false};
    if(opcode==98 || opcode==213 || opcode==15) return inventory_packet(cfg,arena,opcode,&r);
    if(opcode==193) {
        if(length) return -1;
        memset(varps->varps,0,sizeof(varps->varps));return 1;
    }
    if(opcode==150 || opcode==175) {
        if(length!=(opcode==150?3u:6u)) return -1;
        int id=read_be(&r,2);int value=opcode==150?(int8_t)read_be(&r,1):(int32_t)read_be(&r,4);
        if(id>=MAX_VARPS) return -1;
        varp_set(varps,id,value);return 1;
    }
    if(opcode==44) {
        if(length!=6) return -1;
        int skill=read_be(&r,1);uint32_t xp=read_be(&r,4);int level=read_be(&r,1);
        if(skill>=25 || xp>INT_MAX/10) return -1;
        varps->skill_levels[skill]=level;varps->skill_exp[skill]=(int)xp*10;
        int points=0,base=1;
        for(int i=1;i<=99;i++) {
            if(xp>=(uint32_t)(points/4)) base=i;
            points+=(int)(i+300.0*pow(2.0,i/7.0));
        }
        varps->skill_base_levels[skill]=base;return 1;
    }
    if(opcode==68) {if(length!=1) return -1;varps->energy=data[0];return 1;}
    if(opcode==22) {if(length!=2) return -1;varps->weight=(int16_t)read_be(&r,2);return 1;}
    size_t expected=opcode==2?4:opcode==26?3:opcode==209?6:0;
    if(!expected && opcode!=201) return 0;
    if((expected && length!=expected) || length<2) return -1;
    int id=read_be(&r,2);Component *c=component_get(cfg,id);if(!c) return -1;
    if(opcode==2) {
        uint32_t color=read_be(&r,2);
        c->colour=((color>>10)&31)<<19 | ((color>>5)&31)<<11 | (color&31)<<3;
    } else if(opcode==26) c->hide=read_be(&r,1)==1;
    else if(opcode==209) {c->x=(int16_t)read_be(&r,2);c->y=(int16_t)read_be(&r,2);}
    else if(opcode==201) {
        if(length<3 || data[length-1]!=10 || memchr(data+2,10,length-3)) return -1;
        char *text=arena_alloc(arena,length-1);if(!text) return -1;
        memcpy(text,data+2,length-3);text[length-3]=0;c->text=text;
    }
    return 1;
}
