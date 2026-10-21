#include "ui/client_actions.h"
#include "ui/component_packets.h"
#include "ui/mouse_picking.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_int(FILE *f) {
    uint8_t b[4];assert(fread(b,1,4,f)==4);
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static void inventory(ComponentConfig *cfg, VarpContext *vars, Arena *arena, bool source) {
    uint8_t data[32];Packet p=packet_wrap(data,sizeof(data));p2(&p,3214);p1(&p,5);
    for(int i=0;i<5;i++) {p2(&p,i==3?996:i==4 && source?1206:0);p1(&p,i>=3?1:0);}
    assert(component_apply_packet(cfg,vars,arena,98,data,p.pos)==1);
}
int main(void) {
    setbuf(stdout,NULL);Arena arena=arena_create(96*1024*1024);
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);
    Jagfile *media=jagfile_load_file("../engine/data/pack/client/media",&arena);
    Jagfile *interfaces=jagfile_load_file("../engine/data/pack/client/interface",&arena);assert(config && media && interfaces);
    ComponentConfig cfg;PixFont *fonts[4]={0};assert(component_unpack(&cfg,media,fonts,interfaces,&arena));
    ObjConfig objects;obj_unpack(&objects,config,true);NpcConfig npcs;npc_unpack(&npcs,config);
    LocConfig locs;loc_unpack(&locs,config);VarpContext vars;varp_init(&vars);
    inventory(&cfg,&vars,&arena,true);MenuSelection selection={0};cfg.selection=&selection;
    EntitySyncContext *entities=calloc(1,sizeof(*entities));World3D *scene=calloc(1,sizeof(*scene));assert(entities && scene);
    entity_sync_init(entities,NULL);world3d_init(scene,104,104,4);
    entities->local_player.base.path_tile_x[0]=entities->local_player.base.path_tile_z[0]=50;
    entities->npcs[42]=calloc(1,sizeof(NpcEntity));assert(entities->npcs[42]);npc_entity_init(entities->npcs[42]);
    entities->npcs[42]->has_type=npc_get(&npcs,0,&entities->npcs[42]->type);assert(entities->npcs[42]->has_type);
    entities->npcs[42]->base.path_tile_x[0]=entities->npcs[42]->base.path_tile_z[0]=50;
    entities->players[17]=calloc(1,sizeof(PlayerEntity));assert(entities->players[17]);player_entity_init(entities->players[17]);
    entities->players[17]->visible=true;snprintf(entities->players[17]->name,32,"Fixture");
    entities->players[17]->base.path_tile_x[0]=entities->players[17]->base.path_tile_z[0]=50;
    assert(world3d_add_map_location(scene,(WorldMapLocation){.level=0,.x=50,.z=50,.id=1530,.shape=0,.width=1,.length=1,.active=true}));
    GroundItem *ground=calloc(1,sizeof(*ground));assert(ground);ground->id=995;ground->count=1;scene->tiles[0][50][50].items=ground;
    CollisionMap collision[4];for(int i=0;i<4;i++) collision_map_init(&collision[i],104,104);
    Pathfinder pf;pathfinder_init(&pf);MapStreamer map={.base_tile_x=3184,.base_tile_z=3184};
    assert(client_stream_global_init());socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr *)&address,sizeof(address))==0 && listen(listener,1)==0);
#ifdef _WIN32
    int address_size=sizeof(address);
#else
    socklen_t address_size=sizeof(address);
#endif
    assert(getsockname(listener,(struct sockaddr *)&address,&address_size)==0);
    GameSession session;protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)));
    socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);session.in_game=true;
    ClickFeedback cross={0};int selected_tab=6;
    ClientActions ctx={.session=&session,.components=&cfg,.objects=&objects,.selection=&selection,.entities=entities,
        .scene=scene,.locations=&locs,.pathfinder=&pf,.collision=collision,.map=&map,.live=true,
        .click_feedback=&cross,.click_x=256,.click_y=167,.selected_tab=&selected_tab};
    FILE *f=fopen("tests/fixtures/target-reference.bin","rb");assert(f);
    assert(read_int(f)==0x54415231 && read_int(f)==9);
    const int pick_types[]={1,2,3,0},ids[]={42,1530,995,17};
    const MenuActionType item_types[]={MENU_ITEM_ON_NPC,MENU_ITEM_ON_LOC,MENU_ITEM_ON_OBJ,MENU_ITEM_ON_PLAYER};
    const MenuActionType spell_types[]={MENU_SPELL_ON_NPC,MENU_SPELL_ON_LOC,MENU_SPELL_ON_OBJ,MENU_SPELL_ON_PLAYER};
    for(int i=0;i<9;i++) {
        assert(read_int(f)==(uint32_t)i);int spell=read_int(f),expected_cross=read_int(f),length=read_int(f);
        uint8_t expected[64],actual[64];assert(length>0 && length<64 && fread(expected,1,length,f)==(size_t)length);
        uint32_t seed[]={17,31,57,99};isaac_init(&session.out_cipher,seed,4);cross.mode=CLICK_NONE;
        if(i<4) assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_ITEM_SELECT,.target=1205,.slot=4,.component=3214})==ACTION_LOCAL);
        else assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_SPELL_SELECT,.component=spell})==ACTION_LOCAL);
        ContextMenu menu;context_menu_init(&menu);
        if(i==8) {
            assert(selected_tab==3 && selection.spell_selected);
            Component *root=component_get(&cfg,3213),*grid=component_get(&cfg,3214);assert(root && grid);
            int gx=-1,gy=-1;
            for(int child=0;child<root->child_count;child++) if(root->child_id[child]==3214) {gx=root->child_x[child];gy=root->child_y[child];}
            assert(gx>=0 && gy>=0);
            int x=562+gx+3*(grid->margin_x+32)+grid->inv_slot_offset_x[3];
            int y=231+gy+grid->inv_slot_offset_y[3];
            assert(component_build_menu(&cfg,root,x+8,y+8,562,231,0,&menu,&objects));
        } else {
            int kind=i%4;MousePicker picker;mouse_picker_init(&picker);picker.selection=&selection;
            mouse_picker_add_picked(&picker,mouse_picker_make_bitset(pick_types[kind],ids[kind],50,50));
            mouse_picker_build_live_menu(&picker,&menu,&npcs,&locs,&objects,entities);
            assert(menu.size==2); // Cancel plus exactly the selected operation; no Walk/Examine/ordinary op.
            assert(menu.actions[menu.size-1].type==(i<4?item_types[kind]:spell_types[kind]));
        }
        assert(menu.size>0 && client_action_dispatch(&ctx,menu.actions[menu.size-1])==ACTION_SENT);
        assert(cross.mode==expected_cross && !selection.item_selected && !selection.spell_selected);
        int got=0;while(got<length) {int bytes=recv(peer,(char *)actual+got,length-got,0);assert(bytes>0);got+=bytes;}
        assert(!memcmp(expected,actual,length));
        printf("PASS original Java target case=%d spell=%d bytes=%d cross=%d\n",i,spell,length,cross.mode);
    }
    assert(fgetc(f)==EOF);fclose(f);
    assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_ITEM_SELECT,.target=1205,.slot=4,.component=3214})==ACTION_LOCAL);
    inventory(&cfg,&vars,&arena,false);Isaac before=session.out_cipher;cross.mode=CLICK_NONE;
    assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_ITEM_ON_NPC,.target=42})==ACTION_FAILED);
    assert(!memcmp(&before,&session.out_cipher,sizeof(before)) && cross.mode==CLICK_NONE);
    assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_SPELL_SELECT,.component=1152})==ACTION_LOCAL);
    assert(!(selection.spell_flags&4));
    assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_SPELL_ON_LOC,.target=1530,.x=50,.z=50})==ACTION_FAILED);
    assert(!memcmp(&before,&session.out_cipher,sizeof(before)));
    assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_CANCEL})==ACTION_LOCAL);
    assert(!selection.item_selected && !selection.spell_selected);
    protocol_close_session(&session);
#ifdef _WIN32
    closesocket(peer);closesocket(listener);
#else
    close(peer);close(listener);
#endif
    client_stream_global_cleanup();entity_sync_cleanup(entities);free(entities);world3d_dispose(scene);free(scene);
    obj_free(&objects);npc_free(&npcs);loc_free(&locs);arena_free(&arena);
    puts("PASS: real-cache selection/menu/dispatch matches all 9 original encrypted item/spell target streams; stale sources and invalid target masks send nothing");
    return 0;
}
