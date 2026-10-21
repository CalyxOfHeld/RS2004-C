#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ui/component_packets.h"

int main(void) {
    puts("[Component packets] Production handler tests with explicit wire fixtures; not live gameplay");
    Arena arena=arena_create(4*1024*1024);
    ComponentConfig cfg={0};cfg.count=6000;
    cfg.instances=arena_alloc_zero(&arena,cfg.count*sizeof(Component));
    for(int i=0;i<cfg.count;i++) cfg.instances[i].id=i;
    Component *inv=&cfg.instances[3214];inv->type=COMPONENT_TYPE_INV_GRID;inv->width=4;inv->height=7;
    VarpContext vars;varp_init(&vars);
    uint8_t full[]={12,142,2, 4,254,1, 3,228,255,0,0,195,80};
    assert(component_apply_packet(&cfg,&vars,&arena,98,full,sizeof(full))==1);
    assert(inv->inv_slot_obj_id[0]==1278 && inv->inv_slot_obj_count[0]==1);
    assert(inv->inv_slot_obj_id[1]==996 && inv->inv_slot_obj_count[1]==50000);
    uint8_t partial[]={12,142,17, 4,148,3};
    assert(component_apply_packet(&cfg,&vars,&arena,213,partial,sizeof(partial))==1);
    assert(inv->inv_slot_obj_id[17]==1172 && inv->inv_slot_obj_count[17]==3);
    uint8_t shorter[]={12,142,1, 0,0,0};
    assert(component_apply_packet(&cfg,&vars,&arena,98,shorter,sizeof(shorter))==1);
    for(int i=0;i<28;i++) assert(inv->inv_slot_obj_id[i]==0 && inv->inv_slot_obj_count[i]==0);
    assert(component_apply_packet(&cfg,&vars,&arena,98,full,sizeof(full))==1);
    uint8_t malformed[]={12,142,0, 0,0,0, 1,0,1,255,0};
    assert(component_apply_packet(&cfg,&vars,&arena,213,malformed,sizeof(malformed))==-1);
    assert(inv->inv_slot_obj_id[0]==1278 && inv->inv_slot_obj_count[1]==50000);
    uint8_t invalid_slot[]={12,142,28,0,1,1};
    assert(component_apply_packet(&cfg,&vars,&arena,213,invalid_slot,sizeof(invalid_slot))==-1);
    uint8_t clear[]={12,142,0,0,0,0};
    assert(component_apply_packet(&cfg,&vars,&arena,213,clear,sizeof(clear))==1);
    assert(inv->inv_slot_obj_id[0]==0);
    puts("PASS INV-01: full/partial stacks, sparse slots, trailing-slot clearing and atomic malformed rejection");
    uint8_t small[]={0,43,2};
    assert(component_apply_packet(&cfg,&vars,&arena,150,small,sizeof(small))==1 && varp_get(&vars,43)==2);
    uint8_t negative[]={0,44,255};
    assert(component_apply_packet(&cfg,&vars,&arena,150,negative,sizeof(negative))==1 && varp_get(&vars,44)==-1);
    uint8_t large[]={1,44,0,1,134,160};
    assert(component_apply_packet(&cfg,&vars,&arena,175,large,sizeof(large))==1 && varp_get(&vars,300)==100000);
    uint8_t stat[]={0,1,49,45,0,99}; // Synthetic wire fixture: 20,000,000 displayed XP
    assert(component_apply_packet(&cfg,&vars,&arena,44,stat,sizeof(stat))==1);
    assert(vars.skill_levels[0]==99 && vars.skill_base_levels[0]==99);
    uint8_t energy[]={63},weight[]={255,246};
    assert(component_apply_packet(&cfg,&vars,&arena,68,energy,sizeof(energy))==1 && vars.energy==63);
    assert(component_apply_packet(&cfg,&vars,&arena,22,weight,sizeof(weight))==1 && vars.weight==-10);
    uint8_t text[]={19,20,'H','a','n','s',10};
    assert(component_apply_packet(&cfg,&vars,&arena,201,text,sizeof(text))==1);
    assert(!strcmp(cfg.instances[4884].text,"Hans"));
    assert(component_apply_packet(&cfg,&vars,&arena,201,text,sizeof(text)-1)==-1);
    assert(!strcmp(cfg.instances[4884].text,"Hans"));
    uint8_t hide[]={19,20,1};
    assert(component_apply_packet(&cfg,&vars,&arena,26,hide,sizeof(hide))==1 && cfg.instances[4884].hide);
    uint8_t position[]={19,20,255,253,0,5};
    assert(component_apply_packet(&cfg,&vars,&arena,209,position,sizeof(position))==1);
    assert(cfg.instances[4884].x==-3 && cfg.instances[4884].y==5);
    uint8_t color[]={19,20,124,0};
    assert(component_apply_packet(&cfg,&vars,&arena,2,color,sizeof(color))==1 && cfg.instances[4884].colour==0xf80000);
    assert(component_apply_packet(&cfg,&vars,&arena,150,small,2)==-1 && varp_get(&vars,43)==2);
    assert(component_apply_packet(&cfg,&vars,&arena,193,NULL,0)==1 && varp_get(&vars,43)==0);
    assert(component_apply_packet(&cfg,&vars,&arena,237,NULL,0)==0);
    puts("PASS: actual UI receive path handles varps, stats, energy, weight, text delimiters, hide, signed offsets and RGB conversion");
    arena_free(&arena);puts("[Component packets] PASS");return 0;
}

