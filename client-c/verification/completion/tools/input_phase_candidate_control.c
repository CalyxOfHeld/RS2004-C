#ifdef INPUT_PHASE_BASELINE
#include "../runs/2026-10-26-input-phase-sol/client-before.c"
#else
#include "../../../src/client/client.c"
#endif
#include <assert.h>
static int phase_mode;
static void phase_check(bool condition,const char *what) {
    if(!condition){fprintf(stderr,"FAIL input phase mode%d: %s\n",phase_mode,what);exit(1);}
}
static int phase_int(FILE *f) {
    uint32_t v=0;for(int i=0;i<4;i++){int b=fgetc(f);phase_check(b!=EOF,"fixture read");v=(v<<8)|(unsigned)b;}return (int32_t)v;
}
static void phase_event(ClientEventType type,int button) {
    ClientEvent e={.type=type,.button=button,.x=100,.y=100};client_queue_event(&e);
}
int main(int argc,char **argv) {
    phase_check(argc==2,"reference argument");
    FILE *f=fopen(argv[1],"rb");phase_check(f!=NULL,"reference open");
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;
    ClientPlatform platform={0};phase_check(client_init(&options,&platform),"real-cache init");midi_set_silent(true);
    phase_check(client_stream_global_init(),"socket init");
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);phase_check(listener!=INVALID_SOCKET_VAL,"listener");
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    phase_check(!bind(listener,(struct sockaddr*)&address,sizeof(address)) && !listen(listener,1),"ephemeral listen");
    int size=sizeof(address);phase_check(!getsockname(listener,(struct sockaddr*)&address,&size),"port");
    protocol_init_session(&session);phase_check(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)),"connect");
    socket_t peer=accept(listener,NULL,NULL);phase_check(peer!=INVALID_SOCKET_VAL,"accept");closesocket(listener);
    DWORD timeout=2000;setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
    session.in_game=true;session.stream.buffered=true;ingame=live_server=true;offline=false;actions.live=true;
    player.visible=false;world3d_reset(world3d);world_reset(world);streamer.scene_state=0;scene_build_pending=false;
    int count=phase_int(f);phase_check(count==10,"case count");
    for(int record=0;record<count;record++) {
        phase_mode=phase_int(f);int length=phase_int(f);uint8_t expected[64],actual[64];
        phase_check(length>0 && length<=64 && fread(expected,1,(size_t)length,f)==(size_t)length,"wire fixture");
        int expected_counter=phase_int(f),expected_modal=fgetc(f),expected_drag=fgetc(f),expected_menu=fgetc(f);
        uint32_t seed[]={225,26,10,3};isaac_init(&session.out_cipher,seed,4);
        heartbeat_timer=cyclelogic2=cyclelogic4=cyclelogic5=cyclelogic6=idle_cycles=idle_timeout=0;
        camera_offset_cycle=minimap_offset_cycle=0;session.idle_net_cycles=0;cyclelogic3=127;cyclelogic1=150;
        interfaces.viewport=interfaces.sidebar=interfaces.chat=-1;selected_tab=-1;selection.spell_selected=selection.item_selected=false;
        comp_cfg.drag.active=false;comp_cfg.scroll_drag.active=false;varp_ctx.mouse_buttons_option=0;
        mouse_x=mouse_y=100;modal_message225[0]=0;menu.visible=false;menu_opened_cycle225=false;
        world_action_pending225=minimap_click_pending225=false;cycle_click_button=0;key_event_count225=0;
        context_menu_clear(&menu);context_menu_add_action(&menu,"Read",(MenuAction){.type=MENU_IF_BUTTON,.component=673,.operation=1});
        if(phase_mode==1 || phase_mode==7)strcpy(modal_message225,"Message");
        if(phase_mode==2 || phase_mode==3 || phase_mode==9) {
            comp_cfg.drag.active=true;comp_cfg.drag.cycles=0;comp_cfg.drag.threshold=false;comp_cfg.drag.grab_x=comp_cfg.drag.grab_y=100;
        }
        if(phase_mode==5)phase_event(CLIENT_EVENT_MOUSE_DOWN,CLIENT_BUTTON_RIGHT);
        if(phase_mode!=8)phase_event(CLIENT_EVENT_MOUSE_DOWN,CLIENT_BUTTON_LEFT);
        if(phase_mode==6 || phase_mode==7)phase_event(CLIENT_EVENT_MOUSE_DOWN,CLIENT_BUTTON_RIGHT);
        if(phase_mode==2 || phase_mode==4 || phase_mode==8 || phase_mode==9)phase_event(CLIENT_EVENT_MOUSE_UP,CLIENT_BUTTON_LEFT);
        client_process_events();phase_check(session.stream.out_len==0,"GameShell event collection sends no action packet");
        phase_check(client_update() && ingame && !reconnecting && !session.protocol_error,"actual updateGame");
        int received=0;while(received<length){int n=recv(peer,(char*)actual+received,length-received,0);phase_check(n>0,"wire receive");received+=n;}
        phase_check(!memcmp(actual,expected,(size_t)length),"encrypted packet bytes and order match original Java");
        fd_set ready;FD_ZERO(&ready);FD_SET(peer,&ready);struct timeval wait={0,0};phase_check(select(0,&ready,NULL,NULL,&wait)==0,"no extra packets");
        phase_check(cyclelogic1==expected_counter,"consumed-click counter");
        phase_check((modal_message225[0]!=0)==expected_modal && comp_cfg.drag.active==expected_drag && menu.visible==expected_menu,"modal/drag/menu state");
    }
    fclose(f);
    /* Exercise the queued five-cycle swap, not only the short-release branch.
     * The cache supplies the inventory geometry; expected wire is the original
     * updateGame INV_BUTTOND tuple, followed by CYCLELOGIC3. */
    phase_mode=10;
    Component *root=component_get(&comp_cfg,3213),*inv=component_get(&comp_cfg,3214);
    phase_check(root && inv && inv->draggable && inv->inv_slot_obj_id,"cached inventory");
    int px[2]={0},py[2]={0};
    for(int y=231;y<461;y++)for(int x=562;x<752;x++) {
        int slot=-1;Component *hit=component_find_inv_slot(&comp_cfg,root,x,y,562,231,0,&slot);
        if(hit==inv && slot>=0 && slot<2 && !px[slot]){px[slot]=x+10;py[slot]=y+10;}
    }
    phase_check(px[0] && px[1],"inventory slot positions");
    inv->inv_slot_obj_id[0]=1;inv->inv_slot_obj_count[0]=3;inv->inv_slot_obj_id[1]=2;inv->inv_slot_obj_count[1]=7;
    interfaces.sidebar=3213;selected_tab=3;modal_message225[0]=0;menu.visible=false;
    comp_cfg.drag.active=false;cycle_click_button=0;world_action_pending225=false;
    cyclelogic1=cyclelogic3=cyclelogic4=cyclelogic5=cyclelogic6=heartbeat_timer=0;
    uint32_t drag_seed[]={225,26,10,4};isaac_init(&session.out_cipher,drag_seed,4);
    context_menu_clear(&menu);context_menu_add_action(&menu,"Use",(MenuAction){.type=MENU_HELD_OP,.component=3214,.slot=0,.operation=1});
    ClientEvent move={.type=CLIENT_EVENT_MOUSE_MOVE,.x=px[0],.y=py[0]};client_queue_event(&move);client_process_events();
    ClientEvent down={.type=CLIENT_EVENT_MOUSE_DOWN,.button=CLIENT_BUTTON_LEFT,.x=px[0],.y=py[0]};client_queue_event(&down);client_process_events();
    phase_check(client_update() && comp_cfg.drag.active && comp_cfg.drag.cycles==0,"press starts drag without an early cycle");
    move.x=px[1];move.y=py[1];client_queue_event(&move);client_process_events();
    for(int i=1;i<=4;i++)phase_check(client_update() && comp_cfg.drag.cycles==i && inv->inv_slot_obj_id[0]==1,"held drag retains slots");
    down.x=px[1];down.y=py[1];client_queue_event(&down);
    ClientEvent up=down;up.type=CLIENT_EVENT_MOUSE_UP;client_queue_event(&up);client_process_events();
    phase_check(session.stream.out_len==0 && inv->inv_slot_obj_id[0]==1,"release is latched until updateGame");
    cyclelogic3=127;phase_check(client_update() && !comp_cfg.drag.active,"fifth-cycle release");
    phase_check(inv->inv_slot_obj_id[0]==2 && inv->inv_slot_obj_count[0]==7 && inv->inv_slot_obj_id[1]==1 && inv->inv_slot_obj_count[1]==3,"IDs and counts swap together");
    uint8_t wire[11];int received=0;while(received<11){int n=recv(peer,(char*)wire+received,11-received,0);phase_check(n>0,"drag wire");received+=n;}
    Isaac cipher;isaac_init(&cipher,drag_seed,4);
    phase_check((uint8_t)(wire[0]-isaac_next(&cipher))==159 && wire[1]==12 && wire[2]==142 && wire[3]==0 && wire[4]==0 && wire[5]==0 && wire[6]==1,"original INV_BUTTOND fields");
    phase_check((uint8_t)(wire[7]-isaac_next(&cipher))==215 && ((wire[8]<<16)|(wire[9]<<8)|wire[10])==4991788,"drag packet precedes CYCLELOGIC3");
    phase_check(cyclelogic1==1,"release consumes newly latched press");
    closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();
    puts("PASS 10 real-cache candidate updateGame cases: exact original encrypted wire/order, press coalescing, drag/modal consumption and menu state; additional five-cycle inventory swap sends before CYCLELOGIC3; isolated socket, no account.");return 0;
}
