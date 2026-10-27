#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static void report_key_event(int key) {
    ClientEvent e={0};if(key==8){e.type=CLIENT_EVENT_KEY_DOWN;e.key=CLIENT_KEY_DELETE;}else if(key==10 || key==13){e.type=CLIENT_EVENT_KEY_DOWN;e.key=CLIENT_KEY_RETURN;}else if(key>=32 && key<=255){e.type=CLIENT_EVENT_TEXT;e.text[0]=(char)key;}else return;
    client_queue_event(&e);client_process_events();for(int i=0;i<key_event_count225;i++)client_handle_event(&key_events225[i]);key_event_count225=0;
}
static void report_click(int x,int y){ClientEvent e={0};e.type=CLIENT_EVENT_MOUSE_DOWN;e.button=CLIENT_BUTTON_LEFT;e.x=x;e.y=y;client_queue_event(&e);client_process_events();client_mouse_input225();cycle_click_button=0;}
static bool report_find(Component *root,int x,int y,int code,int *mx,int *my) {
    if(!root || root->hide || !root->child_id)return false;
    for(int i=0;i<root->child_count;i++){Component *c=component_get(&comp_cfg,root->child_id[i]);if(!c || c->hide)continue;int cx=x+root->child_x[i]+c->x,cy=y+root->child_y[i]+c->y-root->scroll_position;if(c->client_code==code && (code!=0 || c->button_type==3)){*mx=cx+2;*my=cy+2;return true;}if(c->type==0 && report_find(c,cx,cy,code,mx,my))return true;}return false;
}
int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f && read_int(f)==0x52413235);int cases=read_int(f),root=read_int(f);assert(cases==2182);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);DWORD timeout=2000;assert(setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))==0);
    session.in_game=true;session.stream.buffered=true;live_server=true;ingame=true;actions.live=true;isaac_init(&session.out_cipher,seed,4);int reports=0,transmissions=0;
    for(int record=0;record<cases;record++) {
        int mode=read_int(f);char name[256];assert(overhead_read_text(f,name,256));int rights=fgetc(f),toggles=read_int(f),end=read_int(f),x=read_int(f),y=read_int(f),n=read_int(f),keys[256];assert(n>=0 && n<256);for(int i=0;i<n;i++)keys[i]=read_int(f);
        report_setup(&chatbox,mode,name,root,&interfaces.viewport,&interfaces.chat,&interfaces.sidebar);session.is_mod=rights!=0;assert(!session.stream.out_len);memset(session.stream.out_buf,0xa5,5000);context_menu_clear(&menu);menu.visible=false;context_menu_add_action(&menu,"Cancel",(MenuAction){.type=MENU_CANCEL});modal_message225[0]=0;
        if(mode==0)report_click(x,y);
        if(mode==1){char label[320];snprintf(label,sizeof(label),!strcmp(name,"<missing-tag>")?"Report abuse plain":"Report abuse @whi@  %s  ",name);ContextMenu selected;context_menu_init(&selected);context_menu_add_option(&selected,label,record%2==0?2034:34);assert(client_dispatch_action(selected.actions[0])==ACTION_LOCAL);}
        for(int i=0;i<n;i++)report_key_event(keys[i]);
        for(int i=0;i<toggles;i++)assert(client_dispatch_action((MenuAction){.type=MENU_IF_BUTTON,.component=report_component(&comp_cfg,613)})==ACTION_LOCAL);
        if(end==13)assert(client_dispatch_action((MenuAction){.type=MENU_IF_CLOSE})==ACTION_SENT);
        else if(end){if(chatbox.report_input[0])reports++;assert(client_dispatch_action((MenuAction){.type=MENU_IF_BUTTON,.component=report_component(&comp_cfg,600+end)})==ACTION_LOCAL);}
        bool full=record==2145;if(full)assert(client_update());
        int bytes=read_int(f);uint8_t expected[32];assert(bytes>=0 && bytes<=32 && fread(expected,1,bytes,f)==(size_t)bytes);assert((full?session.stream.out_len==0:session.stream.out_len==(size_t)bytes) && !memcmp(session.stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session.stream.out_buf[i]==0xa5);
        report_state(f,&chatbox,&session,interfaces.viewport,interfaces.chat,interfaces.sidebar,record);
        if(bytes){if(!full)assert(client_stream_flush(&session.stream));uint8_t actual[32];for(int got=0;got<bytes;){int n=recv(peer,(char*)actual+got,bytes-got,0);assert(n>0);got+=n;}assert(!memcmp(actual,expected,bytes));transmissions++;}
    }
    assert(fgetc(f)==EOF && reports==1720);fclose(f);
    f=fopen(argv[2],"rb");assert(f && read_int(f)==0x52553235 && read_int(f)==24 && read_int(f)==root);uint32_t *expected=malloc(512*334*sizeof(*expected));assert(expected);int painted=0;
    for(int frame=0;frame<24;frame++) {
        int cycle=read_int(f);char name[256],cursor[258],label[256];assert(overhead_read_text(f,name,256));bool rights=fgetc(f)!=0,muted=fgetc(f)!=0;assert(overhead_read_text(f,cursor,258) && overhead_read_text(f,label,256));uint32_t colour=(uint32_t)read_int(f);for(int i=0;i<512*334;i++)expected[i]=(uint32_t)read_int(f);
        if(frame!=6 && frame!=7)continue;
        interfaces.viewport=root;interfaces.chat=interfaces.sidebar=-1;chatbox.report_interface=root;strcpy(chatbox.report_input,name);if(frame==6)chatbox.report_mute=false;assert(chatbox.report_mute==muted);session.is_mod=rights;game_ticks=cycle;location_anim_cycle=cycle;streamer.scene_state=2;menu.visible=false;mouse_x=mouse_y=0;assert(client_draw());
        assert(!strcmp(comp_cfg.instances[report_component(&comp_cfg,600)].text,cursor));assert(!strcmp(comp_cfg.instances[report_component(&comp_cfg,613)].text,label) && comp_cfg.instances[report_component(&comp_cfg,613)].colour==colour);
        for(int y=0;y<334;y++)for(int x=0;x<512;x++)if(expected[y*512+x]!=0x123456){assert(pixels[(VIEWPORT_Y+y)*CLIENT_WIDTH+VIEWPORT_X+x]==expected[y*512+x]);painted++;}
        int mx,my;assert(report_find(component_get(&comp_cfg,root),VIEWPORT_X,VIEWPORT_Y,frame==6?613:601,&mx,&my));mouse_x=mx;mouse_y=my;assert(client_draw());report_click(mx,my);
        if(frame==6)assert(chatbox.report_mute && !session.stream.out_len);else {assert(interfaces.viewport==-1 && session.stream.out_len==12);assert(client_update());uint8_t actual[12];for(int got=0;got<12;){int n=recv(peer,(char*)actual+got,12-got,0);assert(n>0);got+=n;}}
    }
    assert(client_report_open(&actions,"Alice",false)==ACTION_LOCAL);chatbox.count_dialog_open=true;int close_x,close_y;assert(report_find(component_get(&comp_cfg,root),VIEWPORT_X,VIEWPORT_Y,0,&close_x,&close_y));mouse_x=close_x;mouse_y=close_y;assert(client_draw());report_click(close_x,close_y);assert(interfaces.viewport==-1 && !chatbox.count_dialog_open && session.stream.out_len==2);assert(client_stream_flush(&session.stream));uint8_t closed[2];for(int got=0;got<2;){int n=recv(peer,(char*)closed+got,2-got,0);assert(n>0);got+=n;}
    assert(fgetc(f)==EOF && painted>1000);fclose(f);free(expected);closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();printf("PASS candidate report:2182 original cases,%d isolated transmissions,1720 category reports; actual footer/deferred editor/full-update send; cached mute/category/cancel clicks and%d original panel foreground pixels; no production connection\n",transmissions,painted);return 0;
}
