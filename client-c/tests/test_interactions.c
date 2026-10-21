#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "ui/component.h"
#include "ui/component_packets.h"
#include "ui/client_actions.h"
#include "config/obj.h"
#include "config/idk.h"

int main(void) {
    Arena arena=arena_create(96*1024*1024);
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);
    Jagfile *media=jagfile_load_file("../engine/data/pack/client/media",&arena);
    Jagfile *interfaces=jagfile_load_file("../engine/data/pack/client/interface",&arena);
    assert(config && media && interfaces);
    ComponentConfig cfg;PixFont *fonts[4]={0};
    assert(component_unpack(&cfg,media,fonts,interfaces,&arena));
    ObjConfig objects;obj_unpack(&objects,config,true);
    IdkConfig kits;idk_unpack(&kits,config,&arena);
    VarpContext vars;varp_init(&vars);MenuSelection selection={0};cfg.selection=&selection;
    ContextMenu menu;context_menu_init(&menu);
    Component *root=component_get(&cfg,3213);assert(root && root->child_count>0);
    int gx=-1,gy=-1;
    for(int i=0;i<root->child_count;i++) {
        printf("[CACHE] inventory child=%d offset=(%d,%d)\n",root->child_id[i],root->child_x[i],root->child_y[i]);
        if(root->child_id[i]==3214) {gx=root->child_x[i];gy=root->child_y[i];}
    }
    assert(gx>=0 && gy>=0);
    uint8_t inventory[]={12,142,2, 4,254,1,4,182,1}; // explicit fixtures, cache IDs 1277 and 1205
    assert(component_apply_packet(&cfg,&vars,&arena,98,inventory,sizeof(inventory))==1);
    Component *grid=component_get(&cfg,3214);assert(grid && grid->interactable);
    int sx=562+gx+(grid->margin_x+32)+grid->inv_slot_offset_x[1];
    int sy=231+gy+grid->inv_slot_offset_y[1];
    assert(component_build_menu(&cfg,root,sx+8,sy+8,562,231,0,&menu,&objects));
    assert(menu.size>0);
    MenuAction top=menu.actions[menu.size-1];
    for(int i=0;i<menu.size;i++) printf("[MENU] %s type=%d op=%d component=%d slot=%d item=%d\n",menu.options[i],menu.actions[i].type,menu.actions[i].operation,menu.actions[i].component,menu.actions[i].slot,menu.actions[i].target);
    assert(top.type==MENU_HELD_OP && top.operation==2 && top.target==1205 && top.slot==1 && top.component==3214);
    puts("PASS UI-02: real cache inventory hit-test preserves the second slot, item, component and Wield operation");
    // IDs stay independent from action types even when larger than the former ranges.
    for(int i=0;i<cfg.count;i++) {
        Component *com=component_get(&cfg,i);
        if(com && com->button_type && com->script_count && com->scripts && com->scripts[0] && com->script_lengths[0]>=2 && com->scripts[0][0]==5 && com->scripts[0][1]==43)
            printf("[CACHE] combat-style component=%d button=%d operand=%d layer=%d\n",i,com->button_type,com->script_operand?com->script_operand[0]:-1,com->layer);
    }
    context_menu_clear(&menu);
    MenuAction high={.type=MENU_IF_BUTTON,.component=3651,.operation=1};context_menu_add_action(&menu,"Accept",high);
    assert(menu.actions[0].type==MENU_IF_BUTTON && menu.actions[0].component==3651);
    puts("PASS UI-01: typed high-ID widget action has no numeric range collision");
    context_menu_clear(&menu);
    context_menu_add_action(&menu,"Talk-to Cook",(MenuAction){.type=MENU_NPC_OP,.operation=1,.target=5793});
    context_menu_add_action(&menu,"Examine Cooking pot",(MenuAction){.type=MENU_LOC_EXAMINE,.target=1});
    context_menu_prioritize(&menu);
    assert(menu.actions[menu.size-1].type==MENU_NPC_OP);
    puts("PASS: low-priority scenery Examine cannot override a normal NPC operation");
    ClientDesign design;client_design_init(&design,&kits);
    for(int i=0;i<7;i++) assert(design.parts[i]>=0 && !kits.instances[design.parts[i]].disable && kits.instances[design.parts[i]].type==i);
    puts("PASS: design defaults are selected from enabled real cache identity-kit types");
    arena_free(&arena);puts("[Interactions] PASS (production menu/cache checks; live round trip is separate)");return 0;
}
