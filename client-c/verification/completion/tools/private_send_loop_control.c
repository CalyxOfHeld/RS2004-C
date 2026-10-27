/* Actual candidate menu dispatch/deferred keyboard phase and isolated transport. */
#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static bool find_friend_row(Component *root,int x,int y,int *mx,int *my) {
    if(!root || root->hide || !root->child_id)return false;
    for(int i=0;i<root->child_count;i++) {
        Component *child=component_get(&comp_cfg,root->child_id[i]);if(!child || child->hide)continue;
        int cx=x+root->child_x[i]+child->x,cy=y+root->child_y[i]+child->y-root->scroll_position;
        if(child->client_code==1){*mx=cx+2;*my=cy+2;return true;}
        if(child->type==COMPONENT_TYPE_CONTAINER && find_friend_row(child,cx,cy,mx,my))return true;
    }
    return false;
}
static void private_key(int key,bool drain) {
    ClientEvent event={0};
    if(key==8){event.type=CLIENT_EVENT_KEY_DOWN;event.key=CLIENT_KEY_DELETE;}
    else if(key==10 || key==13){event.type=CLIENT_EVENT_KEY_DOWN;event.key=CLIENT_KEY_RETURN;}
    else if(key>=32 && key<=255){event.type=CLIENT_EVENT_TEXT;event.text[0]=(char)key;}
    else return;
    client_queue_event(&event);client_process_events();
    if(drain){for(int i=0;i<key_event_count225;i++)client_handle_event(&key_events225[i]);key_event_count225=0;}
}
int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f && read_int(f)==0x50563235);int cases=read_int(f);assert(cases==1987);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);
    DWORD timeout=2000;assert(setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))==0);
    session.in_game=true;session.stream.buffered=true;live_server=true;ingame=true;actions.live=true;isaac_init(&session.out_cipher,seed,4);chatbox_free(&chatbox);chatbox_init(&chatbox,"Self");int transmissions=0;
    const char *names[]={"Alice","Bob","Self Test","Missing"};
    for(int record=0;record<cases;record++) {
        public_chat_setting=read_int(f);private_chat_setting=read_int(f);trade_chat_setting=read_int(f);int who=read_int(f),blocked=fgetc(f);assert(who>=0 && who<4 && blocked>=0);
        chatbox.public_mode=public_chat_setting;chatbox.private_mode=private_chat_setting;chatbox.trade_mode=trade_chat_setting;interfaces.chat=blocked?27:-1;
        private_send_prepare(&chatbox);assert(!session.stream.out_len);memset(session.stream.out_buf,0xa5,5000);
        ActionResult result=client_action_dispatch(&actions,(MenuAction){.type=MENU_PRIVATE_MESSAGE,.name37=jstring_to_base37(names[who])});assert(result==((who==0||who==2)?ACTION_LOCAL:ACTION_UNAVAILABLE));private_send_state(f,&chatbox,record);
        int keys=read_int(f);assert(keys>=0 && keys<256);bool full=record==cases-1;
        for(int i=0;i<keys;i++)private_key(read_int(f),!full);
        if(full){assert(key_event_count225>0 && chatbox.private_prompt_open && !session.stream.out_len);assert(client_update());assert(!key_event_count225);}
        int bytes=read_int(f);assert(bytes>=0 && bytes<256);uint8_t expected[256];assert(fread(expected,1,bytes,f)==(size_t)bytes);
        assert((full?session.stream.out_len==0:session.stream.out_len==(size_t)bytes) && !memcmp(session.stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session.stream.out_buf[i]==0xa5);
        private_send_state(f,&chatbox,record);assert(read_int(f)==public_chat_setting);assert(read_int(f)==private_chat_setting);assert(read_int(f)==trade_chat_setting);
        assert(read_int(f)==session.out_cipher.count);assert((uint32_t)read_int(f)==session.out_cipher.a);assert((uint32_t)read_int(f)==session.out_cipher.b);assert((uint32_t)read_int(f)==session.out_cipher.c);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session.out_cipher.rsl[i]);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session.out_cipher.mem[i]);
        for(int i=0;i<100;i++){assert(read_int(f)==chatbox.type[i]);private_string(f,chatbox.sender[i],record,i,"sender");private_string(f,chatbox.text[i],record,i,"history");}
        if(bytes){if(!full)assert(client_stream_flush(&session.stream));uint8_t actual[256];for(int got=0;got<bytes;){int n=recv(peer,(char*)actual+got,bytes-got,0);assert(n>0);got+=n;}assert(!memcmp(actual,expected,bytes));transmissions++;}
    }
    assert(fgetc(f)==EOF && transmissions==1746);fclose(f);
    /* The actual friend tab supplies the primary Message action to a real click. */
    streamer.scene_state=1;interfaces.chat=-1;interfaces.viewport=-1;interfaces.sidebar=-1;selected_tab=8;menu.visible=false;
    chatbox.private_prompt_open=false;chatbox.count_dialog_open=false;modal_message225[0]=0;mouse_x=mouse_y=0;assert(client_draw());
    int mx,my;assert(find_friend_row(component_get(&comp_cfg,5065),562,231,&mx,&my));mouse_x=mx;mouse_y=my;assert(client_draw());
    assert(menu.size>=3 && menu.actions[menu.size-1].type==MENU_PRIVATE_MESSAGE && menu.actions[menu.size-1].name37==chatbox.friend_ids[0]);
    ClientEvent click={0};click.type=CLIENT_EVENT_MOUSE_DOWN;click.button=CLIENT_BUTTON_LEFT;click.x=mx;click.y=my;client_queue_event(&click);client_process_events();assert(client_update());assert(chatbox.private_prompt_open && chatbox.private_recipient==chatbox.friend_ids[0]);
    strcpy(chatbox.private_input,"Hello WORLD!");chatbox.private_input_len=12;strcpy(modal_message225,"hidden");chatbox.count_dialog_open=true;mouse_x=mouse_y=0;assert(client_draw());
    uint32_t expected_prompt[479*96]={0};pix2d_bind(expected_prompt,479,96);pix8_draw(media.chatback,0,0);chatbox_draw_private_prompt(&chatbox,0,0,&font_b12);
    for(int y=0;y<96;y++)assert(!memcmp(pixels+(375+y)*CLIENT_WIDTH+22,expected_prompt+y*479,479*sizeof(*pixels)));
    pix2d_bind(pixels,CLIENT_WIDTH,CLIENT_HEIGHT);
    /* Original incoming243 replaces the social prompt with amount input. */
    uint32_t incoming_seed[]={225,243,1,0};Isaac incoming;isaac_init(&incoming,incoming_seed,4);isaac_init(&session.in_cipher,incoming_seed,4);uint8_t frame=(uint8_t)(243+isaac_next(&incoming));
    assert(send(peer,(char*)&frame,1,0)==1 && client_stream_wait_readable(&session.stream,1000)==1);assert(client_update());assert(!chatbox.private_prompt_open && chatbox.count_dialog_open && !chatbox.count_dialog_input[0]);
    closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();
    puts("PASS candidate private send:1987 actual menu/deferred event cases,1746 isolated loopback transmissions,last through full client_update; actual friend-tab click/full prompt pixels; incoming243 replaces prompt; prompt/input/bytes/cipher/history/privacy exact");return 0;
}
