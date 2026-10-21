#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/zone_sync.h"
#include "ui/client_actions.h"
#include "ui/mouse_picking.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
static World *world;
static World3D *scene;
static LocConfig locs;
static CollisionMap collision[4];
static ZoneSync zone;
static void packet(int op,const uint8_t *data,size_t length) {
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,op,data,length)==1);
}
static void item(int op,int id,int count,int next) {
    uint8_t data[]={0x22,id>>8,id,count>>8,count,next>>8,next};
    packet(op,data,op==49?3:op==223?5:7);
}
static int entries(GroundTile *tile) {int n=0;for(GroundItem *p=tile->items;p;p=p->next) n++;return n;}
static void receive(socket_t peer,uint8_t *out,int length) {
    int n=0;while(n<length) {int got=recv(peer,(char *)out+n,length-n,0);assert(got>0);n+=got;}
}
int main(void) {
    Arena arena=arena_create(64*1024*1024);pix3d_init();pix3d_init_colour_table(0.8);
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);
    Jagfile *models=jagfile_load_file("../engine/data/pack/client/models",&arena);assert(config && models);
    model_init_repo(models,&arena);loc_unpack(&locs,config);
    ObjConfig objects;obj_unpack(&objects,config,true);zone.objects=&objects;zone.local_pid=7;
    world=calloc(1,sizeof(*world));scene=calloc(1,sizeof(*scene));assert(world && scene);
    world_init(world,104,104);world3d_init(scene,104,104,4);
    for(int l=0;l<4;l++) collision_map_init(&collision[l],104,104);
    uint8_t base[]={48,48};packet(7,base,2);GroundTile *tile=&scene->tiles[0][50][50];
    item(223,1205,1,0);item(223,995,10000,0);
    assert(entries(tile)==2 && tile->items->id==995 && tile->items->model);
    Model *coin_model=tile->items->model;
    item(50,1277,1,7);assert(entries(tile)==2);
    item(50,1277,1,8);assert(entries(tile)==3);
    item(223,995,5,0);assert(entries(tile)==4);
    GroundItem *selected[3];assert(ground_items_models(tile,selected)==3);
    assert(selected[0]->id==995 && selected[1]->id==1205 && selected[2]->id==1277);
    item(151,995,9999,1);assert(tile->items->model==coin_model && tile->items->count==10000);
    item(151,995|32768,10000,1);assert(tile->items->id==1277);
    item(49,995|32768,0,0);assert(entries(tile)==3);
    int coins=0;for(GroundItem *p=tile->items;p;p=p->next) if(p->id==995) {coins++;assert(p->count==5);}assert(coins==1);
    uint8_t malformed[]={48,48,223,0x22,4,181,0,1,151,0x22};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,malformed,sizeof(malformed))==-1 && entries(tile)==3);
    uint8_t badid[]={48,48,223,0x22,4,181,0,1,223,0x22,127,255,0,1};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,badid,sizeof(badid))==-1 && entries(tile)==3);
    assert(ground_items_event(&scene->tiles[1][50][50],&objects,223,1205,1,0));
    packet(135,base,2);assert(!tile->items && scene->tiles[1][50][50].items);
    uint8_t edge[]={103,103};packet(7,edge,2);item(223,1205,1,0);assert(!scene->tiles[0][103][103].items);
    packet(135,edge,2);packet(7,base,2);item(223,1205,1,0);
    GroundItem *original=tile->items;GroundItem **saved=ground_items_detach(scene);assert(saved);
    world3d_reset(scene);ground_items_restore(scene,saved,8,-8);
    assert(scene->tiles[0][42][58].items==original && !tile->items);
    saved=ground_items_detach(scene);world3d_reset(scene);ground_items_restore(scene,saved,-8,8);assert(tile->items==original);
    puts("PASS: ADD/DEL/COUNT/REVEAL, duplicates, owner suppression, values, malformed atomic rejection, full-zone planes, edges and region rebase");
    uint32_t *pixels=calloc(512*334,sizeof(*pixels));assert(pixels);pix2d_bind(pixels,512,334);
    pix3d_center_x=256;pix3d_center_y=167;MousePicker picker;mouse_picker_init(&picker);scene->picker=&picker;
    bool picked=false;int px=0,py=0;
    for(int y=100;y<260 && !picked;y+=2) for(int x=220;x<300 && !picked;x+=2) {
        memset(pixels,0,512*334*sizeof(*pixels));mouse_picker_reset(&picker,x,y);
        world3d_render(scene,6464,-500,5864,256,0,0,0);pix3d_end_scene();
        for(int i=0;i<picker.picked_count;i++) if((picker.picked_bitsets[i]>>29)==3) {picked=true;px=x;py=y;}
    }
    assert(picked);int lit=0;for(int i=0;i<512*334;i++) if(pixels[i]) lit++;assert(lit>0);
    ContextMenu menu;context_menu_init(&menu);mouse_picker_build_menu(&picker,&menu,NULL,&locs,&objects,NULL,0,NULL,0,3);
    MenuAction take={0};for(int i=0;i<menu.size;i++) if(menu.actions[i].type==MENU_OBJ_OP) take=menu.actions[i];
    assert(take.operation==3 && take.target==1205 && take.x==50 && take.z==50);
    printf("PASS: actual cache model rendered %d pixels; triangle pick (%d,%d) produces Take\n",lit,px,py);
    assert(client_stream_global_init());socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr *)&address,sizeof(address))==0 && listen(listener,1)==0);
#ifdef _WIN32
    int size=sizeof(address);
#else
    socklen_t size=sizeof(address);
#endif
    assert(getsockname(listener,(struct sockaddr *)&address,&size)==0);
    GameSession session;protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)));
    socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);session.in_game=true;
    uint32_t seed[]={1,2,3,4};Isaac decoder;isaac_init(&session.out_cipher,seed,4);isaac_init(&decoder,seed,4);
    EntitySyncContext *entities=calloc(1,sizeof(*entities));assert(entities);entity_sync_init(entities,NULL);
    entities->local_player.base.path_tile_x[0]=48;entities->local_player.base.path_tile_z[0]=50;
    Pathfinder pathfinder;pathfinder_init(&pathfinder);MapStreamer map={.base_tile_x=3200,.base_tile_z=3200};
    MenuSelection selection={0};ClientActions actions={.session=&session,.entities=entities,.scene=scene,.map=&map,
        .pathfinder=&pathfinder,.collision=collision,.selection=&selection,.live=true};
    assert(client_action_dispatch(&actions,take)==ACTION_SENT);
    uint8_t wire[32];receive(peer,wire,2);assert((uint8_t)(wire[0]-isaac_next(&decoder))==93);
    int length=wire[1];assert(length<20);receive(peer,wire,length);receive(peer,wire,7);
    assert((uint8_t)(wire[0]-isaac_next(&decoder))==200);
    assert(((wire[1]<<8)|wire[2])==3250 && ((wire[3]<<8)|wire[4])==3250 && ((wire[5]<<8)|wire[6])==1205);
    item(49,1205,0,0);assert(client_action_dispatch(&actions,take)==ACTION_FAILED);
    puts("PASS: picked Take sends approach 93 then OPOBJ3 200 with world coordinates/id; removed target rejected");
    protocol_close_session(&session);
#ifdef _WIN32
    closesocket(peer);closesocket(listener);
#else
    close(peer);close(listener);
#endif
    free(entities);free(pixels);zone_sync_dispose(&zone);world3d_dispose(scene);free(scene);free(world);
    obj_free(&objects);loc_free(&locs);arena_free(&arena);return 0;
}
