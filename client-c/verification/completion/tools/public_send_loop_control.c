/* Actual candidate event path, real cache, isolated socket. Original fixture
 * supplies encrypted bytes/full state; no copied prefix/filter/echo algorithm. */
#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static void send_key_control(int key,bool drain_phase) {
    ClientEvent event={0};
    if(key==8){event.type=CLIENT_EVENT_KEY_DOWN;event.key=CLIENT_KEY_DELETE; /* GameShell maps physical Delete to code8 too. */}
    else if(key==13 || key==10){event.type=CLIENT_EVENT_KEY_DOWN;event.key=CLIENT_KEY_RETURN;}
    else if(key>=32 && key<=255){event.type=CLIENT_EVENT_TEXT;event.text[0]=(char)key;}
    else return;
    client_queue_event(&event);client_process_events();
    if(drain_phase) {
        /* Same source event phase that client_update runs after chat expiry. */
        for(int i=0;i<key_event_count225;i++)client_handle_event(&key_events225[i]);
        key_event_count225=0;
    }
}
int main(int argc,char **argv) {
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f && read_int(f)==0x50533235);int cases=read_int(f);assert(cases==3521);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;ClientPlatform platform={0};assert(client_init(&options,&platform));assert(client_stream_global_init());
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(listener!=INVALID_SOCKET_VAL);struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0 && listen(listener,1)==0);int size=sizeof(addr);assert(getsockname(listener,(struct sockaddr*)&addr,&size)==0);
    protocol_init_session(&session);assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(addr.sin_port)));socket_t peer=accept(listener,NULL,NULL);assert(peer!=INVALID_SOCKET_VAL);closesocket(listener);
    DWORD timeout=2000;assert(setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))==0);
    session.in_game=true;session.stream.buffered=true;live_server=true;ingame=true;isaac_init(&session.out_cipher,seed,4);chatbox_free(&chatbox);chatbox_init(&chatbox,"Alice");int transmissions=0;
    for(int record=0;record<cases;record++) {
        public_chat_setting=read_int(f);private_chat_setting=read_int(f);trade_chat_setting=read_int(f);int blocked=fgetc(f),n=read_int(f);assert(blocked>=0 && n>=0 && n<256);
        chatbox.public_mode=public_chat_setting;chatbox.private_mode=private_chat_setting;chatbox.trade_mode=trade_chat_setting;interfaces.chat=blocked?27:-1;
        player_entity_init(&player);strcpy(player.name,"Alice");strcpy(player.base.chat,"old");player.base.chat_color=13;player.base.chat_style=17;player.base.chat_timer=7;
        chatbox.input[0]=0;chatbox.input_len=0;assert(session.stream.out_len==0);memset(session.stream.out_buf,0xa5,5000);
        bool full_update=record==cases-1;
        for(int i=0;i<n;i++)send_key_control(read_int(f),!full_update);
        if(full_update){assert(key_event_count225>0 && !session.stream.out_len && !strcmp(player.base.chat,"old"));assert(client_update());assert(key_event_count225==0);}
        int bytes=read_int(f);assert(bytes>=0 && bytes<=512);uint8_t expected[512];assert(fread(expected,1,bytes,f)==(size_t)bytes);
        if((!full_update && session.stream.out_len!=(size_t)bytes) || memcmp(session.stream.out_buf,expected,bytes))fprintf(stderr,"Candidate public-send wire mismatch record%d\n",record);
        assert((full_update?session.stream.out_len==0:session.stream.out_len==(size_t)bytes) && !memcmp(session.stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session.stream.out_buf[i]==0xa5);
        private_string(f,chatbox.input,record,-1,"input");assert(read_int(f)==public_chat_setting);assert(read_int(f)==private_chat_setting);assert(read_int(f)==trade_chat_setting);
        private_string(f,player.base.chat,record,-1,"actor");assert(read_int(f)==player.base.chat_color);assert(read_int(f)==player.base.chat_style);assert(read_int(f)==player.base.chat_timer);
        assert(read_int(f)==session.out_cipher.count);assert((uint32_t)read_int(f)==session.out_cipher.a);assert((uint32_t)read_int(f)==session.out_cipher.b);assert((uint32_t)read_int(f)==session.out_cipher.c);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session.out_cipher.rsl[i]);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session.out_cipher.mem[i]);
        for(int i=0;i<100;i++){assert(read_int(f)==chatbox.type[i]);private_string(f,chatbox.sender[i],record,i,"sender");private_string(f,chatbox.text[i],record,i,"history");}
        if(bytes){if(!full_update)assert(client_stream_flush(&session.stream));uint8_t received[512];for(int got=0;got<bytes;){int count=recv(peer,(char*)received+got,bytes-got,0);assert(count>0);got+=count;}assert(!memcmp(received,expected,bytes));transmissions++;}
    }
    assert(fgetc(f)==EOF);fclose(f);assert(transmissions==3295);
    closesocket(peer);protocol_close_session(&session);live_server=false;client_shutdown();client_stream_global_cleanup();
    puts("PASS candidate public send:3521 deferred event-phase cases (last through full client_update) and3295 isolated loopback transmissions; original encrypted bytes/full cipher/history/actor/input/settings exact; modal blocking and bounded keys");return 0;
}
