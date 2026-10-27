#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static void social_key(int key,bool drain) {
    ClientEvent e={0};
    if(key==8){e.type=CLIENT_EVENT_KEY_DOWN;e.key=CLIENT_KEY_DELETE;}
    else if(key==10 || key==13){e.type=CLIENT_EVENT_KEY_DOWN;e.key=CLIENT_KEY_RETURN;}
    else if(key>=32 && key<=255){e.type=CLIENT_EVENT_TEXT;e.text[0]=(char)key;}else return;
    client_queue_event(&e);client_process_events();if(drain){for(int i=0;i<key_event_count225;i++)client_handle_event(&key_events225[i]);key_event_count225=0;}
}
static bool find_social_button(Component *root,int x,int y,int code,int *mx,int *my) {
    if(!root || root->hide || !root->child_id)return false;
    for(int i=0;i<root->child_count;i++){Component *c=component_get(&comp_cfg,root->child_id[i]);if(!c || c->hide)continue;int cx=x+root->child_x[i]+c->x,cy=y+root->child_y[i]+c->y-root->scroll_position;
        if(c->client_code==code){*mx=cx+2;*my=cy+2;return true;}if(c->type==0 && find_social_button(c,cx,cy,code,mx,my))return true;}
    return false;
}
int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f && read_int(f)==0x534f3235);int cases=read_int(f);assert(cases==3266);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);DWORD timeout=2000;assert(setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))==0);
    session.in_game=true;session.stream.buffered=true;live_server=true;ingame=true;actions.live=true;isaac_init(&session.out_cipher,seed,4);int transmissions=0,errors=0;
    for(int record=0;record<cases;record++) {
        int friends=read_int(f),ignores=read_int(f),kind=read_int(f),self=read_int(f),action=read_int(f),form=read_int(f);uint64_t id=private_u64(f);int n=read_int(f),keys[256];assert(n>=0 && n<256);for(int i=0;i<n;i++)keys[i]=read_int(f);
        social_prepare(&chatbox,friends,ignores,kind);strcpy(player.name,self==0?"Self Test":self==1?"SELF TEST":"");interfaces.chat=27;assert(!session.stream.out_len);memset(session.stream.out_buf,0xa5,5000);bool full=record==cases-2;
        if(form==0)assert(chatbox_social_action(&chatbox,&session,action,id,self==2?NULL:player.name));
        else if(form==1){int code=action==1?406:action==2?557:action==4?436:556;char name[32],label[80];jstring_from_base37(id,name,32);if(!id)name[0]=0;snprintf(label,80,"Action @whi@  %s  ",name);ContextMenu menu;context_menu_init(&menu);context_menu_add_option(&menu,label,code+(record%2==0?2000:0));assert(client_dispatch_action(menu.actions[0])==ACTION_LOCAL);}
        else {int code=action==1?201:action==2?202:action==4?501:502,button=-1;for(int i=0;i<comp_cfg.count;i++)if(comp_cfg.instances[i].client_code==code){button=i;break;}assert(button>=0);assert(client_dispatch_action((MenuAction){.type=MENU_IF_BUTTON,.operation=1,.component=button})==ACTION_LOCAL);
            for(int i=0;i<n;i++)social_key(keys[i],!full);
            if(full){assert(key_event_count225 && !session.stream.out_len);assert(client_update());assert(!key_event_count225);}}
        int bytes=read_int(f);uint8_t expected[64];assert(bytes>=0 && bytes<=64 && fread(expected,1,bytes,f)==(size_t)bytes);
        assert((full?session.stream.out_len==0:session.stream.out_len==(size_t)bytes) && !memcmp(session.stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session.stream.out_buf[i]==0xa5);
        if(chatbox.text[0] && strncmp(chatbox.text[0],"old text",8))errors++;
        social_state(f,&chatbox,&session,record);
        if(bytes){if(!full)assert(client_stream_flush(&session.stream));uint8_t actual[64];for(int got=0;got<bytes;){int n=recv(peer,(char*)actual+got,bytes-got,0);assert(n>0);got+=n;}assert(!memcmp(actual,expected,bytes));transmissions++;}
    }
    assert(fgetc(f)==EOF && transmissions==758 && errors==532);fclose(f);
    /* Real cached Add/Remove buttons, queued mouse clicks and complete prompt draw. */
    social_prepare(&chatbox,3,3,1);strcpy(player.name,"Self Test");streamer.scene_state=1;interfaces.chat=interfaces.sidebar=interfaces.viewport=-1;modal_message225[0]=0;
    int expected_actions[]={1,2,4,5};
    for(int a=0;a<4;a++) {
        int code=(int[]){201,202,501,502}[a],tab=a<2?8:9,root=a<2?5065:5715,mx,my;selected_tab=tab;menu.visible=false;mouse_x=mouse_y=0;assert(client_draw());assert(find_social_button(component_get(&comp_cfg,root),562,231,code,&mx,&my));mouse_x=mx;mouse_y=my;assert(client_draw());
        ClientEvent e={0};e.type=CLIENT_EVENT_MOUSE_DOWN;e.button=CLIENT_BUTTON_LEFT;e.x=mx;e.y=my;client_queue_event(&e);client_process_events();assert(client_update());assert(chatbox.private_prompt_open && chatbox.social_action==expected_actions[a]);
        uint32_t expected[479*96]={0};mouse_x=mouse_y=0;assert(client_draw());pix2d_bind(expected,479,96);pix8_draw(media.chatback,0,0);chatbox_draw_private_prompt(&chatbox,0,0,&font_b12);for(int y=0;y<96;y++)assert(!memcmp(pixels+(375+y)*CLIENT_WIDTH+22,expected+y*479,479*sizeof(*pixels)));pix2d_bind(pixels,CLIENT_WIDTH,CLIENT_HEIGHT);
        chatbox.private_prompt_open=false;
    }
    closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();puts("PASS candidate social:3266 original cases,758 loopback transmissions,532 error histories; full cipher/list tails/prompt/input exact; full-update positive send;4 actual friend/ignore tab button clicks/full prompt images");return 0;
}
