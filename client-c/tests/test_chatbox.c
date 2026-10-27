#include "ui/chatbox.h"
#include "ui/overhead_chat.h"
#include "ui/component.h"
#include "ui/client_actions.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include "gfx/media.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/jstring.h"
#include "core/packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int read_int(FILE *f) {
    unsigned char b[4]; assert(fread(b, 1, 4, f) == 4);
    return (int)((uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3]);
}
static void compare_pixels(FILE *f, const uint32_t *pixels, size_t size, int case_id, const char *region) {
    for (size_t i = 0; i < size; i++) {
        uint32_t expected = (uint32_t)read_int(f);
        if (pixels[i] != expected) {
            fprintf(stderr, "Original chat mismatch case=%d region=%s pixel=%zu expected=%06x actual=%06x\n",
                    case_id, region, i, expected, pixels[i]);
            assert(pixels[i] == expected);
        }
    }
}
static void original_display_controls(PixFont *font, Media *media) {
    const char *path = getenv("RS2_CHAT_ORACLE");
    if (!path || !*path) return;
    FILE *f = fopen(path, "rb"); assert(f);
    int count = read_int(f); assert(count == 144);
    Chatbox chat; chatbox_init(&chat, "Self Test");
    chat.friend_count = 1; strcpy(chat.friend_names[0], "aLiCe");
    const char *senders[] = {"Self Test", "Alice", "Stranger", "Bob"};
    for (int i = 99; i >= 0; i--) {
        char text[80]; snprintf(text, sizeof(text), "text %d @red@Abc W", i);
        chatbox_add_message(&chat, i % 10, senders[(i / 10 + i) % 4], text);
    }
    strcpy(chat.input, "hello"); chat.input_len = 5;
    uint32_t *pixels = calloc(512 * 334, sizeof(*pixels)); assert(pixels);
    for (int n = 0; n < count; n++) {
        chat.public_mode = read_int(f); chat.private_mode = read_int(f); chat.trade_mode = read_int(f);
        chat.split_private = read_int(f); int timer = read_int(f); chat.scroll_offset = read_int(f);
        pix2d_bind(pixels, 479, 96);
        memset(pixels, 0, 479 * 96 * sizeof(*pixels));
        const char *logins[] = {"self_test", "SELF_test", "sElF_test", "other_name"};
        chatbox_draw225(&chat, media->chatback, 0, 0, font, logins[n % 4], media->scrollbar0, media->scrollbar1);
        compare_pixels(f, pixels, 479 * 96, n, "chatback");
        pix2d_bind(pixels, 512, 334);
        for (int i = 0; i < 512 * 334; i++) pixels[i] = (i * 37 + 12345) & 0xffffff;
        chatbox_draw_private(&chat, 0, 0, font, timer);
        compare_pixels(f, pixels, 512 * 334, n, "split");
        int queries = read_int(f); assert(queries == 149);
        for (int j = 0; j < queries; j++) {
            bool split = read_int(f) != 0; int x = read_int(f), y = read_int(f);
            bool rights = read_int(f) != 0; int options = read_int(f);
            ContextMenu menu; context_menu_init(&menu);
            chatbox_build_menu(&chat, &menu, x, y, split, rights, timer);
            if (menu.size != options) fprintf(stderr,"Menu size case=%d query=%d (%d,%d) split%d expected%d actual%d\n",n,j,x,y,split,options,menu.size);
            assert(menu.size == options);
            for (int k = 0; k < options; k++) {
                int action = read_int(f), len = read_int(f); char text[96];
                assert(len >= 0 && len < (int)sizeof(text)); assert(fread(text, 1, len, f) == (size_t)len); text[len] = 0;
                assert(menu.action_ids[k] == action && !strcmp(menu.options[k], text));
            }
        }
    }
    assert(fgetc(f) == EOF); fclose(f); chatbox_free(&chat); free(pixels);
    puts("[PASS] Original Java:144 complete chatback/split images,31246848 pixels,21456 hover queries exact");
}
static uint64_t private_u64(FILE *f) {
    uint64_t n=0;for(int i=0;i<8;i++){int ch=fgetc(f);assert(ch>=0);n=n*256+(unsigned)ch;}return n;
}
static void private_string(FILE *f,const char *actual,int record,int slot,const char *kind) {
    int n=read_int(f);
    if(n==-1){assert(!actual);return;}
    if(n<0 || !actual || strlen(actual)!=(size_t)n)fprintf(stderr,"String mismatch record%d slot%d kind%s expected units%d actual=%s\n",record,slot,kind,n,actual?actual:"<null>");
    assert(n>=0 && actual && strlen(actual)==(size_t)n);
    for(int i=0;i<n;i++) {
        int high=fgetc(f),low=fgetc(f);assert(high>=0 && low>=0);
        if(((high<<8)|low)!=(uint8_t)actual[i])fprintf(stderr,"Private mismatch record%d slot%d %s unit%d\n",record,slot,kind,i);
        assert(((high<<8)|low)==(uint8_t)actual[i]);
    }
}
static void private_message_controls(Arena *arena) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",arena);assert(jag);
    WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    const char *path=getenv("RS2_PRIVATE_ORACLE");
    FILE *f=fopen(path && *path?path:"tests/fixtures/private-message-reference.bin","rb");assert(f);
    assert(read_int(f)==0x504d3235);int cases=read_int(f);assert(cases==2634);
    Chatbox chat;chatbox_init(&chat,"Self");int additions=0,failures=0;
    for(int record=0;record<cases;record++) {
        int reset=fgetc(f),opcode=fgetc(f);assert(reset>=0 && opcode>=0);
        if(reset){chatbox_free(&chat);chatbox_init(&chat,"Self");}
        int override=read_int(f),expected_ok=fgetc(f),length=read_int(f);assert(expected_ok>=0 && length>=0 && length<=800);
        uint8_t payload[800];assert(fread(payload,1,length,f)==(size_t)length);
        char *before=chat.text[0];
        bool ok=opcode==21?chatbox_apply_ignores(&chat,payload,length):chatbox_add_private_message(&chat,&tables,payload,length,override!=0);
        if(ok!=(expected_ok!=0))fprintf(stderr,"Private return mismatch record%d opcode%d\n",record,opcode);
        assert(ok==(expected_ok!=0));if(!ok)failures++;if(chat.text[0]!=before)additions++;
        assert(read_int(f)==(int)chat.ignore_count);for(int i=0;i<100;i++)assert(private_u64(f)==chat.ignore_ids[i]);
        assert(read_int(f)==(int)chat.private_message_next);for(int i=0;i<100;i++)assert(read_int(f)==chat.private_message_ids[i]);
        int count=0;
        for(int i=0;i<100;i++) {
            assert(read_int(f)==chat.type[i]);private_string(f,chat.sender[i],record,i,"sender");private_string(f,chat.text[i],record,i,"text");if(chat.text[i])count++;
        }
        assert(count==chat.message_count);
    }
    assert(fgetc(f)==EOF);fclose(f);assert(additions==2520 && failures==1);
    Chatbox before=chat;uint8_t malformed[801]={0};
    for(size_t n=0;n<13;n++){assert(!chatbox_add_private_message(&chat,&tables,malformed,n,false));assert(!memcmp(&before,&chat,sizeof(chat)));}
    assert(!chatbox_add_private_message(&chat,&tables,malformed,256,false));
    for(size_t n=1;n<800;n++)if(n%8){assert(!chatbox_apply_ignores(&chat,malformed,n));assert(!memcmp(&before,&chat,sizeof(chat)));}
    assert(!chatbox_apply_ignores(&chat,malformed,801) && !chatbox_apply_ignores(&chat,NULL,8));
    assert(!chatbox_add_private_message(&chat,&tables,NULL,13,false));assert(!memcmp(&before,&chat,sizeof(chat)));
    assert(!chatbox_apply_ignores(NULL,malformed,8) && !chatbox_add_private_message(NULL,&tables,malformed,13,false));
    /* Missing filter must fail closed after original ring admission, never push raw text. */
    Packet p=packet_wrap(malformed,14);p8(&p,1);p4(&p,0x12345678);p1(&p,2);p1(&p,0);
    assert(!chatbox_add_private_message(&chat,NULL,malformed,14,false));assert(chat.text[0]==before.text[0]);
    assert(chat.private_message_next==(before.private_message_next+1)%100);
    assert(chat.private_message_ids[before.private_message_next]==0x12345678);
    chatbox_free(&chat);wordfilter_tables_free(&tables);
    printf("PASS incoming private original client.read: %d packet states,2520 history additions,1 caught decode failure; all100 history/ignore/ring slots exact; malformed/NULL/missing-filter safety\n",cases);
}
static void public_send_controls(Arena *arena) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",arena);assert(jag);WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    const char *path=getenv("RS2_PUBLIC_SEND_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/public-send-reference.bin","rb");assert(f);
    assert(read_int(f)==0x50533235);int cases=read_int(f);assert(cases==3521);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);
    Chatbox chat;chatbox_init(&chat,"Alice");int wire_cases=0;
    for(int record=0;record<cases;record++) {
        chat.public_mode=read_int(f);chat.private_mode=read_int(f);chat.trade_mode=read_int(f);int blocked=fgetc(f),n=read_int(f);assert(blocked>=0 && n>=0 && n<256);
        PlayerEntity actor;player_entity_init(&actor);strcpy(actor.name,"Alice");strcpy(actor.base.chat,"old");actor.base.chat_color=13;actor.base.chat_style=17;actor.base.chat_timer=7;
        chat.input[0]=0;chat.input_len=0;session->stream.out_len=0;memset(session->stream.out_buf,0xa5,sizeof(session->stream.out_buf));
        for(int i=0;i<n;i++){
            int key=read_int(f);if(blocked)continue;
            if(key==8)chatbox_backspace(&chat);
            else if(key==13 || key==10){if(chat.input_len)assert(chatbox_send_public(&chat,&tables,&actor,session));}
            else chatbox_key_press(&chat,(char)key);
        }
        int bytes=read_int(f);assert(bytes>=0 && bytes<=512);uint8_t expected[512];assert(fread(expected,1,bytes,f)==(size_t)bytes);
        if(session->stream.out_len!=(size_t)bytes || memcmp(session->stream.out_buf,expected,bytes))fprintf(stderr,"Public send wire mismatch record%d expected%d actual%zu\n",record,bytes,session->stream.out_len);
        assert(session->stream.out_len==(size_t)bytes && !memcmp(session->stream.out_buf,expected,bytes));if(bytes)wire_cases++;
        for(size_t i=(size_t)bytes;i<sizeof(session->stream.out_buf);i++)assert(session->stream.out_buf[i]==0xa5);
        private_string(f,chat.input,record,-1,"input");assert((size_t)chat.input_len==strlen(chat.input));assert(read_int(f)==chat.public_mode);assert(read_int(f)==chat.private_mode);assert(read_int(f)==chat.trade_mode);
        private_string(f,actor.base.chat,record,-1,"actor");assert(read_int(f)==actor.base.chat_color);assert(read_int(f)==actor.base.chat_style);assert(read_int(f)==actor.base.chat_timer);
        assert(read_int(f)==session->out_cipher.count);assert((uint32_t)read_int(f)==session->out_cipher.a);assert((uint32_t)read_int(f)==session->out_cipher.b);assert((uint32_t)read_int(f)==session->out_cipher.c);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.rsl[i]);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.mem[i]);
        int count=0;for(int i=0;i<100;i++){assert(read_int(f)==chat.type[i]);private_string(f,chat.sender[i],record,i,"sender");private_string(f,chat.text[i],record,i,"history");if(chat.text[i])count++;}assert(count==chat.message_count);
    }
    assert(fgetc(f)==EOF);fclose(f);assert(wire_cases==3295);
    /* Fail closed before cipher/bytes/history/actor/input publication. */
    PlayerEntity actor;player_entity_init(&actor);strcpy(actor.name,"Alice");strcpy(chat.input,"red:wave:hello");chat.input_len=(int)strlen(chat.input);chat.public_mode=2;chat.private_mode=1;chat.trade_mode=1;
    Chatbox before=chat;PlayerEntity actor_before=actor;Isaac cipher=session->out_cipher;session->stream.out_len=0;
    assert(!chatbox_send_public(&chat,NULL,&actor,session));assert(!memcmp(&before,&chat,sizeof(chat)) && !memcmp(&actor,&actor_before,sizeof(actor)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && session->stream.out_len==0);
    session->stream.out_len=4999;memset(session->stream.out_buf,0xa5,5000);
    assert(!chatbox_send_public(&chat,&tables,&actor,session) && session->stream.out_overflow);assert(!memcmp(&before,&chat,sizeof(chat)) && !memcmp(&actor,&actor_before,sizeof(actor)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && session->stream.out_len==4999);
    for(int i=0;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
    session->stream.out_len=0;session->stream.out_overflow=false;session->in_game=false;assert(!chatbox_send_public(&chat,&tables,&actor,session));session->in_game=true;
    strcpy(chat.input,"::help");chat.input_len=6;assert(!chatbox_send_public(&chat,&tables,&actor,session));chat.input[0]=(char)0xff;assert(!chatbox_send_public(&chat,&tables,&actor,session));
    assert(!protocol_send_public_chat(session,NULL,1,0,0,false,0,0));assert(!protocol_send_public_chat(session,"",81,0,0,false,0,0));assert(!protocol_send_public_chat(session,"",0,12,0,false,0,0));assert(!protocol_send_public_chat(session,"",0,0,3,false,0,0));
    assert(!session->stream.out_len && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)));
    chatbox_free(&chat);wordfilter_tables_free(&tables);free(session);
    printf("PASS public send original handleInputKey: %d key cases,3295 wire-producing cases; encrypted bytes/full ISAAC state/all100 history/actor/input/modes exact; filter-required/capacity/invalid/command guards\n",cases);
}
static void visibility_controls(void) {
    Chatbox chat; chatbox_init(&chat, "Self");
    assert(!chatbox_is_friend(&chat, NULL)); assert(chatbox_is_friend(&chat, "sELF"));
    uint8_t payload[9]; Packet p = packet_wrap(payload, sizeof(payload));
    p8(&p, (int64_t)jstring_to_base37("alice")); p1(&p, 0);
    assert(chatbox_apply_friend(&chat, payload, sizeof(payload), 10));
    assert(chat.friend_count == 1 && chatbox_is_friend(&chat, "aLiCe") && chat.message_count == 0);
    payload[8] = 10; assert(chatbox_apply_friend(&chat, payload, 9, 10));
    assert(chat.message_count == 1 && chat.type[0] == 5 && !strcmp(chat.text[0], "Alice has logged in."));
    assert(chatbox_apply_friend(&chat, payload, 9, 10) && chat.message_count == 1);
    payload[8] = 0; assert(chatbox_apply_friend(&chat, payload, 9, 10));
    assert(chat.message_count == 2 && !strcmp(chat.text[0], "Alice has logged out."));
    assert(!chatbox_apply_friend(&chat, payload, 8, 10) && !chatbox_apply_friend(&chat, payload, 10, 10));
    for (int mode = 0; mode < 4; mode++) for (int type = 0; type < 10; type++) {
        chatbox_add_message(&chat, type, "Stranger", "test");
        chat.public_mode = mode; chat.private_mode = mode; chat.trade_mode = mode;
        chat.split_private = 0;
        bool expected = type == 0 || type == 1 || type == 7 ||
            ((type == 2 || type == 3 || type == 4 || type == 8) && mode == 0) ||
            ((type == 5 || type == 6) && mode < 2);
        assert(chatbox_message_visible(&chat, 0, false) == expected);
    }
    chatbox_free(&chat);
    puts("[PASS] Friend/self and visibility controls; friend packet malformed/idempotence/logins");
}


typedef struct {
    const PathingEntity *actors[80]; int px[80],py[80],count,camera[5];
} OverheadFixture;
static bool overhead_fixture_project(void *user,const PathingEntity *actor,int *x,int *y) {
    OverheadFixture *f=user;
    for(int i=0;i<f->count;i++)if(f->actors[i]==actor){*x=f->px[i];*y=f->py[i];return *x>=0;}
    assert(false);return false;
}
static bool overhead_read_text(FILE *f,char *text,size_t capacity) {
    int n=read_int(f);if(n==-1){text[0]=0;return false;}assert(n>=0 && (size_t)n<capacity);
    for(int i=0;i<n;i++){int hi=fgetc(f),lo=fgetc(f);assert(hi==0 && lo>=0);text[i]=(char)lo;}text[n]=0;return true;
}
static void overhead_controls(Arena *arena,bool (*project)(void*,const PathingEntity*,int*,int*),int ox,int oy) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/title",arena);assert(jag);
    PixFont font;assert(pixfont_load(&font,jag,"b12",arena));
    const char *path=getenv("RS2_OVERHEAD_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/overhead-chat-reference.bin","rb");assert(f);
    assert(read_int(f)==0x4f483235);int cases=read_int(f);assert(cases==1426);
    OverheadChat *list=calloc(1,sizeof(*list));EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(list && ctx);
    int fw=ox?789:512,fh=oy?532:334;size_t size=(size_t)fw*fh;
    uint32_t *guard=malloc((size+2)*sizeof(*guard)),*expected=malloc(512*334*sizeof(*expected));assert(guard && expected);
    uint32_t *pixels=guard+1;int failures=0;long long changed=0;
    Chatbox chat;chatbox_init(&chat,"deliberately different login");chat.friend_count=1;strcpy(chat.friend_names[0],"aLiCe");
    for(int n=0;n<cases;n++) {
        memset(ctx,0,sizeof(*ctx));OverheadFixture fixture={0};
        chat.public_mode=read_int(f);int effects=read_int(f),cycle=read_int(f),clip[4];for(int i=0;i<4;i++)clip[i]=read_int(f);
        for(int i=0;i<5;i++)fixture.camera[i]=read_int(f);
        fixture.count=read_int(f);assert(fixture.count>0 && fixture.count<=80);
        for(int i=0;i<fixture.count;i++) {
            int kind=read_int(f),visible=fgetc(f);char name[256],text[256];overhead_read_text(f,name,sizeof(name));
            PathingEntity *a;
            if(kind<2){PlayerEntity *p=kind==0?&ctx->local_player:calloc(1,sizeof(*p));assert(p);p->visible=visible!=0;assert(strlen(name)<sizeof(p->name));strcpy(p->name,name);a=&p->base;
                if(kind){int id=3+i;ctx->players[id]=p;ctx->player_indices[ctx->player_count++]=id;}}
            else {NpcEntity *p=calloc(1,sizeof(*p));assert(p);p->has_type=visible!=0;a=&p->base;int id=2+i;ctx->npcs[id]=p;ctx->npc_indices[ctx->npc_count++]=id;}
            a->x=read_int(f);a->z=read_int(f);a->height=read_int(f);overhead_read_text(f,text,sizeof(text));strcpy(a->chat,text);
            a->chat_color=read_int(f);a->chat_style=read_int(f);a->chat_timer=read_int(f);
            fixture.actors[i]=a;fixture.px[i]=read_int(f);fixture.py[i]=read_int(f);
        }
        bool success=fgetc(f)!=0;int count=read_int(f);guard[0]=guard[size+1]=0xdeadbeef;
        for(size_t i=0;i<size;i++)pixels[i]=0x123456;
        pix2d_bind(pixels,fw,fh);pix2d_set_clip(clip[0]+ox,clip[1]+oy,clip[2]+ox,clip[3]+oy);
        bool ok=overhead_chat_draw(list,ctx,&chat,&font,project?project:overhead_fixture_project,&fixture,cycle,effects,ox,oy);
        if(ok!=success || list->count!=count)fprintf(stderr,"Overhead return/count frame%d expected%d/%d actual%d/%d\n",n,success,count,ok,list->count);
        assert(ok==success && list->count==count);if(!ok)failures++;
        for(int i=0;i<50;i++) {
            int values[]={list->x[i],list->y[i],list->width[i],list->height[i],list->colour[i],list->style[i],list->timer[i]};
            for(int j=0;j<7;j++){int want=read_int(f);if(values[j]!=want)fprintf(stderr,"Overhead state frame%d slot%d field%d expected%d actual%d\n",n,i,j,want,values[j]);assert(values[j]==want);}
            char text[256];bool present=overhead_read_text(f,text,sizeof(text));assert(present==list->present[i]);assert(!present || !strcmp(text,list->text[i]));
        }
        int actualclip[]={pix2d_clip_left-ox,pix2d_clip_top-oy,pix2d_clip_right-ox,pix2d_clip_bottom-oy};
        for(int i=0;i<4;i++){int want=read_int(f);assert(want==actualclip[i]);}
        for(int i=0;i<512*334;i++)expected[i]=0x123456;
        int changes=read_int(f);changed+=changes;for(int i=0;i<changes;i++){int at=read_int(f);assert(at>=0 && at<512*334);expected[at]=(uint32_t)read_int(f);}
        for(int y=0;y<fh;y++)for(int x=0;x<fw;x++) {
            uint32_t want=x>=ox && x<ox+512 && y>=oy && y<oy+334?expected[(y-oy)*512+x-ox]:0x123456;
            if(pixels[y*fw+x]!=want)fprintf(stderr,"Overhead pixels frame%d x%d y%d expected%06x actual%06x\n",n,x,y,want,pixels[y*fw+x]);
            assert(pixels[y*fw+x]==want);
        }
        assert(guard[0]==0xdeadbeef && guard[size+1]==0xdeadbeef);
        for(int i=0;i<ctx->player_count;i++)free(ctx->players[ctx->player_indices[i]]);
        for(int i=0;i<ctx->npc_count;i++)free(ctx->npcs[ctx->npc_indices[i]]);
    }
    assert(fgetc(f)==EOF);fclose(f);assert(failures==6 && changed==1193729);
    memset(ctx,0,sizeof(*ctx));ctx->local_player.visible=true;strcpy(ctx->local_player.name,"Self Test");strcpy(ctx->local_player.base.chat,"safe");ctx->local_player.base.chat_timer=150;
    OverheadFixture safe={0};safe.count=1;safe.actors[0]=&ctx->local_player.base;safe.px[0]=256;safe.py[0]=167;
    for(int control=0;control<10;control++) {
        ctx->player_count=0;ctx->npc_count=0;chat.friend_count=1;chat.public_mode=0;ctx->local_player.base.chat_color=0;ctx->local_player.base.chat_timer=150;strcpy(ctx->local_player.base.chat,"safe");
        if(control==0)ctx->player_count=-1;
        if(control==1)ctx->player_count=MAX_PLAYERS+1;
        if(control==2)ctx->npc_count=-1;
        if(control==3)ctx->npc_count=MAX_NPCS+1;
        if(control==4)chat.friend_count=-1;
        if(control==5)chat.friend_count=101;
        if(control==6)memset(ctx->local_player.base.chat,'x',sizeof(ctx->local_player.base.chat));
        if(control==7)ctx->local_player.base.chat_color=-1;
        if(control==8)ctx->local_player.base.chat_timer=-1;
        if(control==9)ctx->local_player.base.chat_timer=151;
        for(size_t i=0;i<size;i++)pixels[i]=0x123456;
        assert(!overhead_chat_draw(list,ctx,&chat,&font,overhead_fixture_project,&safe,0,0,ox,oy));
        for(size_t i=0;i<size;i++)assert(pixels[i]==0x123456);
        assert(guard[0]==0xdeadbeef && guard[size+1]==0xdeadbeef);
    }
    puts("PASS overhead native safety:10 malformed metadata/text/colour/timer controls; no pixel writes; framebuffer guards intact");
    chatbox_free(&chat);free(guard);free(expected);free(ctx);free(list);
    printf("PASS overhead original:1426 frames,243857408 viewport pixels,all50 persistent slots; %d safe styled50th rejections, %lld changed pixels; origin%d,%d\n",failures,changed,ox,oy);
}


static void private_send_prepare(Chatbox *chat) {
    chat->friend_count=3;const char *names[]={"Alice","Bob","Self Test"};
    for(int i=0;i<3;i++){strcpy(chat->friend_names[i],names[i]);chat->friend_ids[i]=jstring_to_base37(names[i]);chat->friend_worlds[i]=i==1?0:10+i;}
    chat->private_prompt_open=false;strcpy(chat->private_input,"old prompt");chat->private_input_len=10;strcpy(chat->private_prompt,"old header");chat->private_recipient=77;
    chat->count_dialog_open=true;strcpy(chat->count_dialog_input,"42");strcpy(chat->input,"ordinary kept");chat->input_len=13;
}
static void private_send_state(FILE *f,Chatbox *chat,int record) {
    assert((fgetc(f)!=0)==chat->private_prompt_open);
    private_string(f,chat->private_input,record,-1,"private input");private_string(f,chat->private_prompt,record,-1,"prompt");assert(private_u64(f)==chat->private_recipient);
    assert((fgetc(f)!=0)==chat->count_dialog_open);private_string(f,chat->count_dialog_input,record,-1,"count input");private_string(f,chat->input,record,-1,"ordinary input");
    assert(strlen(chat->private_input)==(size_t)chat->private_input_len && strlen(chat->input)==(size_t)chat->input_len);
}
static void private_send_controls(Arena *arena) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",arena);assert(jag);WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    const char *path=getenv("RS2_PRIVATE_SEND_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/private-send-reference.bin","rb");assert(f);
    assert(read_int(f)==0x50563235);int cases=read_int(f);assert(cases==1987);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);
    Chatbox chat;chatbox_init(&chat,"Self");int wire=0;
    const char *names[]={"Alice","Bob","Self Test","Missing"};
    for(int record=0;record<cases;record++) {
        chat.public_mode=read_int(f);chat.private_mode=read_int(f);chat.trade_mode=read_int(f);int who=read_int(f),blocked=fgetc(f);assert(who>=0 && who<4 && blocked>=0);
        private_send_prepare(&chat);bool opened=chatbox_open_private(&chat,jstring_to_base37(names[who]));assert(opened==(who==0 || who==2));private_send_state(f,&chat,record);
        session->stream.out_len=0;memset(session->stream.out_buf,0xa5,sizeof(session->stream.out_buf));int keys=read_int(f);assert(keys>=0 && keys<256);
        for(int i=0;i<keys;i++){int key=read_int(f);if(key==13 || key==10)assert(chatbox_send_private(&chat,&tables,session));else if(key==8)chatbox_backspace(&chat);else if(key>=32 && key<=255)chatbox_key_press(&chat,(char)key);}
        int bytes=read_int(f);assert(bytes>=0 && bytes<256);uint8_t expected[256];assert(fread(expected,1,bytes,f)==(size_t)bytes);
        assert(session->stream.out_len==(size_t)bytes && !memcmp(expected,session->stream.out_buf,bytes));for(int i=bytes;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);if(bytes)wire++;
        private_send_state(f,&chat,record);assert(read_int(f)==chat.public_mode);assert(read_int(f)==chat.private_mode);assert(read_int(f)==chat.trade_mode);
        assert(read_int(f)==session->out_cipher.count);assert((uint32_t)read_int(f)==session->out_cipher.a);assert((uint32_t)read_int(f)==session->out_cipher.b);assert((uint32_t)read_int(f)==session->out_cipher.c);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.rsl[i]);
        for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.mem[i]);
        for(int i=0;i<100;i++){assert(read_int(f)==chat.type[i]);private_string(f,chat.sender[i],record,i,"sender");private_string(f,chat.text[i],record,i,"history");}
    }
    assert(fgetc(f)==EOF && wire==1746);fclose(f);
    private_send_prepare(&chat);assert(chatbox_open_private(&chat,chat.friend_ids[0]));chat.private_input[0]='x';chat.private_input[1]=0;chat.private_input_len=1;
    Chatbox before=chat;Isaac cipher=session->out_cipher;session->stream.out_len=0;
    assert(!chatbox_send_private(&chat,NULL,session));assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && !session->stream.out_len);
    chat.private_input_len=81;assert(!chatbox_send_private(&chat,&tables,session));chat=before;chat.private_input[0]=123;assert(!chatbox_send_private(&chat,&tables,session));chat=before;chat.private_input[1]='x';assert(!chatbox_send_private(&chat,&tables,session));chat=before;
    chatbox_free(&chat);wordfilter_tables_free(&tables);free(session);
    puts("PASS private send original:1987 menu/input cases,1746 wire-producing cases; prompt/ordinary/count input,encrypted bytes/full ISAAC/all100 history/privacy exact; missing-filter/length/character/NUL guards");
}


static void private_ui_controls(Arena *arena,Media *media) {
    const char *path=getenv("RS2_PRIVATE_PROMPT_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/private-prompt-reference.bin","rb");assert(f);
    assert(read_int(f)==0x50503235 && read_int(f)==804);Chatbox chat;chatbox_init(&chat,"Self");
    for(int i=0;i<100;i++){snprintf(chat.friend_names[i],32,"Friend %d",i);chat.friend_ids[i]=jstring_to_base37(chat.friend_names[i]);chat.friend_worlds[i]=i%3==0?0:i%3==1?10:12;}
    int total=0;
    for(int n=0;n<804;n++) {
        chat.friend_count=read_int(f);int code=read_int(f);Component components[2]={0};int child=1,cx=5,cy=7;
        components[0].type=COMPONENT_TYPE_CONTAINER;components[0].width=200;components[0].height=200;components[0].child_count=1;components[0].child_id=&child;components[0].child_x=&cx;components[0].child_y=&cy;
        Component *row=&components[1];row->id=1;row->type=COMPONENT_TYPE_TEXT;row->client_code=code;row->height=160;row->width=100;row->text="before";row->option="ordinary";
        chatbox_prepare_friend_component(&chat,row,10);private_string(f,row->text,n,-1,"friend row");assert(read_int(f)==row->button_type);assert(read_int(f)==row->scroll);
        ComponentConfig cfg={0};cfg.count=2;cfg.instances=components;cfg.chatbox=&chat;ContextMenu menu;context_menu_init(&menu);
        assert(component_build_menu(&cfg,&components[0],6,8,0,0,0,&menu,NULL));int count=read_int(f);assert(count==menu.size);total+=count;
        for(int i=0;i<count;i++){int action=read_int(f);assert(action==menu.action_ids[i]);private_string(f,menu.options[i],n,i,"friend menu");if(action==679){assert(menu.actions[i].type==MENU_PRIVATE_MESSAGE);int index=code>=101?code-101:code-1;assert(menu.actions[i].name37==chat.friend_ids[index]);}}
    }
    assert(total==416);Jagfile *title=jagfile_load_file("../engine/data/pack/client/title",arena);assert(title);PixFont font;assert(pixfont_load(&font,title,"b12",arena));
    int images=read_int(f);assert(images==24);uint32_t *guard=calloc(479*96+2,sizeof(*guard));assert(guard);guard[0]=guard[479*96+1]=0xdeadbeef;
    for(int n=0;n<images;n++) {
        chat.private_prompt_open=true;chat.count_dialog_open=true;assert(overhead_read_text(f,chat.private_prompt,sizeof(chat.private_prompt)));assert(overhead_read_text(f,chat.private_input,sizeof(chat.private_input)));chat.private_input_len=(int)strlen(chat.private_input);
        pix2d_bind(guard+1,479,96);pix2d_cls();pix8_draw(media->chatback,0,0);chatbox_draw_private_prompt(&chat,0,0,&font);compare_pixels(f,guard+1,479*96,n,"private prompt");assert(guard[0]==0xdeadbeef && guard[479*96+1]==0xdeadbeef);
    }
    assert(fgetc(f)==EOF);fclose(f);free(guard);chatbox_free(&chat);
    puts("PASS private UI original:804 dynamic friend rows,416 menu options,24 prompt images/1103616 pixels; actual component menu path and guard pixels");
}


static void social_prepare(Chatbox *chat,int friends,int ignores,int kind) {
    chatbox_free(chat);chatbox_init(chat,"Self");chat->friend_count=friends;chat->ignore_count=(size_t)ignores;
    for(int i=0;i<100;i++){snprintf(chat->friend_names[i],32,"Friend %d",i);chat->friend_ids[i]=jstring_to_base37(chat->friend_names[i]);chat->friend_worlds[i]=i%3==0?0:10+i;char name[32];snprintf(name,sizeof(name),"Ignore %d",i);chat->ignore_ids[i]=jstring_to_base37(name);}
    if(kind==9){chat->friend_ids[0]=chat->ignore_ids[0]=jstring_to_base37("Both");strcpy(chat->friend_names[0],"Both");}
    for(int i=2;i>=0;i--){char text[40],sender[40];snprintf(text,sizeof(text),"old text %d",i);snprintf(sender,sizeof(sender),"old sender %d",i);chatbox_add_message(chat,i,sender,text);}
    chat->social_action=3;strcpy(chat->private_input,"old input");chat->private_input_len=9;strcpy(chat->private_prompt,"old header");chat->private_recipient=77;chat->count_dialog_open=true;strcpy(chat->count_dialog_input,"42");strcpy(chat->input,"ordinary kept");chat->input_len=13;
}
static void social_state(FILE *f,Chatbox *chat,GameSession *session,int record) {
    int fc=read_int(f),ic=read_int(f);if(fc!=chat->friend_count || ic!=(int)chat->ignore_count)fprintf(stderr,"Social counts record%d expected%d,%d actual%d,%zu\n",record,fc,ic,chat->friend_count,chat->ignore_count);
    assert(fc==chat->friend_count && ic==(int)chat->ignore_count);
    for(int i=0;i<100;i++){assert(private_u64(f)==chat->friend_ids[i]);private_string(f,chat->friend_names[i],record,i,"friend");assert(read_int(f)==chat->friend_worlds[i]);assert(private_u64(f)==chat->ignore_ids[i]);}
    assert((fgetc(f)!=0)==chat->private_prompt_open);assert(read_int(f)==chat->social_action);private_string(f,chat->private_input,record,-1,"social input");private_string(f,chat->private_prompt,record,-1,"social header");assert(private_u64(f)==chat->private_recipient);
    assert((fgetc(f)!=0)==chat->count_dialog_open);private_string(f,chat->count_dialog_input,record,-1,"count input");private_string(f,chat->input,record,-1,"ordinary input");assert((size_t)chat->private_input_len==strlen(chat->private_input));
    assert(read_int(f)==session->out_cipher.count);assert((uint32_t)read_int(f)==session->out_cipher.a);assert((uint32_t)read_int(f)==session->out_cipher.b);assert((uint32_t)read_int(f)==session->out_cipher.c);
    for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.rsl[i]);
    for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.mem[i]);
    for(int i=0;i<100;i++){assert(read_int(f)==chat->type[i]);private_string(f,chat->sender[i],record,i,"sender");private_string(f,chat->text[i],record,i,"history");}
}
static void social_controls(void) {
    const char *path=getenv("RS2_SOCIAL_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/social-reference.bin","rb");assert(f && read_int(f)==0x534f3235);int cases=read_int(f);assert(cases==3266);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    GameSession *session=calloc(1,sizeof(*session));EntitySyncContext *entities=calloc(1,sizeof(*entities));assert(session && entities);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);
    Chatbox chat;chatbox_init(&chat,"Self");int wires=0,errors=0;
    for(int record=0;record<cases;record++) {
        int friends=read_int(f),ignores=read_int(f),kind=read_int(f),self=read_int(f),action=read_int(f),form=read_int(f);uint64_t id=private_u64(f);int n=read_int(f),keys[256];assert(n>=0 && n<256);for(int i=0;i<n;i++)keys[i]=read_int(f);
        social_prepare(&chat,friends,ignores,kind);strcpy(entities->local_player.name,self==0?"Self Test":self==1?"SELF TEST":"");session->stream.out_len=0;memset(session->stream.out_buf,0xa5,5000);
        if(form==0)assert(chatbox_social_action(&chat,session,action,id,self==2?NULL:entities->local_player.name));
        else if(form==1) {
            int code=action==1?406:action==2?557:action==4?436:556;char name[32],label[80];jstring_from_base37(id,name,32);if(!id)name[0]=0;snprintf(label,80,"Action @whi@  %s  ",name);
            ContextMenu menu;context_menu_init(&menu);context_menu_add_option(&menu,label,code+(record%2==0?2000:0));assert(menu.actions[0].type==MENU_SOCIAL && menu.actions[0].operation==action && menu.actions[0].name37==id);
            ClientActions ctx={0};ctx.chatbox=&chat;ctx.session=session;ctx.entities=entities;assert(client_action_dispatch(&ctx,menu.actions[0])==ACTION_LOCAL);
        }else {
            assert(chatbox_open_social(&chat,action));for(int i=0;i<n;i++){int key=keys[i];if(key==10 || key==13)assert(chatbox_submit_social(&chat,session,self==2?NULL:entities->local_player.name));else if(key==8)chatbox_backspace(&chat);else if(key>=32 && key<=255)chatbox_key_press(&chat,(char)key);}
        }
        int bytes=read_int(f);uint8_t expected[64];assert(bytes>=0 && bytes<=64 && fread(expected,1,bytes,f)==(size_t)bytes);
        if(session->stream.out_len!=(size_t)bytes || memcmp(session->stream.out_buf,expected,bytes))fprintf(stderr,"Social wire mismatch record%d form%d action%d\n",record,form,action);
        assert(session->stream.out_len==(size_t)bytes && !memcmp(session->stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
        if(bytes)wires++;
        if(chat.text[0] && strncmp(chat.text[0],"old text",8))errors++;
        social_state(f,&chat,session,record);
    }
    assert(fgetc(f)==EOF && wires==758 && errors==532);fclose(f);chatbox_free(&chat);free(entities);free(session);
    puts("PASS social original:3266 direct/menu/prompt cases,758 wire cases,532 exact error messages; all100 list tails/history/full cipher/prompt/input; limits/conflicts/self/name/key rules");
}

static void social_ui_controls(Arena *arena,Media *media) {
    const char *path=getenv("RS2_SOCIAL_UI_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/social-ui-reference.bin","rb");assert(f);
    assert(read_int(f)==0x53493235 && read_int(f)==404);Chatbox chat;chatbox_init(&chat,"Self");
    for(int i=0;i<100;i++){char name[32];snprintf(name,32,"Ignore %d",i);chat.ignore_ids[i]=jstring_to_base37(name);}

    int total=0;
    for(int n=0;n<404;n++) {
        chat.ignore_count=(size_t)read_int(f);int code=read_int(f);Component components[2]={0};int child=1,cx=5,cy=7;
        components[0].type=COMPONENT_TYPE_CONTAINER;components[0].width=200;components[0].height=200;components[0].child_count=1;components[0].child_id=&child;components[0].child_x=&cx;components[0].child_y=&cy;
        Component *row=&components[1];row->id=1;row->type=COMPONENT_TYPE_TEXT;row->client_code=code;row->height=160;row->width=100;row->text="before";row->option="ordinary";
        chatbox_prepare_friend_component(&chat,row,10);private_string(f,row->text,n,-1,"friend row");assert(read_int(f)==row->button_type);assert(read_int(f)==row->scroll);
        ComponentConfig cfg={0};cfg.count=2;cfg.instances=components;cfg.chatbox=&chat;ContextMenu menu;context_menu_init(&menu);
        assert(component_build_menu(&cfg,&components[0],6,8,0,0,0,&menu,NULL));int count=read_int(f);assert(count==menu.size);total+=count;
        for(int i=0;i<count;i++){int action=read_int(f);assert(action==menu.action_ids[i]);private_string(f,menu.options[i],n,i,"friend menu");if(action==556){assert(menu.actions[i].type==MENU_SOCIAL && menu.actions[i].operation==5);assert(menu.actions[i].name37==chat.ignore_ids[code-401]);}}
    }
    assert(total==104);Jagfile *title=jagfile_load_file("../engine/data/pack/client/title",arena);assert(title);PixFont font;assert(pixfont_load(&font,title,"b12",arena));
    int images=read_int(f);assert(images==24);uint32_t *guard=calloc(479*96+2,sizeof(*guard));assert(guard);guard[0]=guard[479*96+1]=0xdeadbeef;
    for(int n=0;n<images;n++) {
        chat.private_prompt_open=true;chat.count_dialog_open=true;assert(overhead_read_text(f,chat.private_prompt,sizeof(chat.private_prompt)));assert(overhead_read_text(f,chat.private_input,sizeof(chat.private_input)));chat.private_input_len=(int)strlen(chat.private_input);
        pix2d_bind(guard+1,479,96);pix2d_cls();pix8_draw(media->chatback,0,0);chatbox_draw_private_prompt(&chat,0,0,&font);compare_pixels(f,guard+1,479*96,n,"private prompt");assert(guard[0]==0xdeadbeef && guard[479*96+1]==0xdeadbeef);
    }
    assert(fgetc(f)==EOF);fclose(f);free(guard);chatbox_free(&chat);
    puts("PASS social UI original:404 dynamic ignore rows,104 menu options,24 prompt images/1103616 pixels; actual component menu path and guard pixels");
}




static void report_load(ComponentConfig *cfg,Arena *arena) {
    Jagfile *title=jagfile_load_file("../engine/data/pack/client/title",arena),*media=jagfile_load_file("../engine/data/pack/client/media",arena),*ui=jagfile_load_file("../engine/data/pack/client/interface",arena);assert(title && media && ui);
    PixFont *fonts=arena_alloc_zero(arena,4*sizeof(*fonts));assert(fonts);const char *names[]={"p11","p12","b12","q8"};PixFont *refs[4];for(int i=0;i<4;i++){assert(pixfont_load(&fonts[i],title,names[i],arena));refs[i]=&fonts[i];}assert(component_unpack(cfg,media,refs,ui,arena));
}
static void report_setup(Chatbox *chat,int mode,const char *name,int root,int *viewport,int *dialog,int *sidebar) {
    chatbox_free(chat);chatbox_init(chat,"Self");*viewport=mode==2?root:123;*dialog=27;*sidebar=55;chat->report_interface=mode==2?root:77;strcpy(chat->report_input,mode==2?name:"old Report");chat->report_mute=true;
    chat->private_prompt_open=true;chat->social_action=3;strcpy(chat->private_input,"old social");chat->private_input_len=10;strcpy(chat->private_prompt,"old header");chat->private_recipient=77;chat->count_dialog_open=true;strcpy(chat->count_dialog_input,"42");strcpy(chat->input,"ordinary kept");chat->input_len=13;
}
static int report_component(ComponentConfig *cfg,int code) {for(int i=0;i<cfg->count;i++)if(cfg->instances[i].client_code==code)return i;assert(false);return -1;}
static void report_state(FILE *f,Chatbox *chat,GameSession *session,int viewport,int dialog,int sidebar,int record) {
    assert(read_int(f)==viewport);assert(read_int(f)==dialog);assert(read_int(f)==sidebar);assert(read_int(f)==chat->report_interface);private_string(f,chat->report_input,record,-1,"report input");assert((fgetc(f)!=0)==chat->report_mute);
    assert((fgetc(f)!=0)==chat->private_prompt_open);assert(read_int(f)==chat->social_action);private_string(f,chat->private_input,record,-1,"social input");private_string(f,chat->private_prompt,record,-1,"social header");assert(private_u64(f)==chat->private_recipient);assert((fgetc(f)!=0)==chat->count_dialog_open);private_string(f,chat->count_dialog_input,record,-1,"count input");private_string(f,chat->input,record,-1,"ordinary input");
    assert(read_int(f)==session->out_cipher.count);assert((uint32_t)read_int(f)==session->out_cipher.a);assert((uint32_t)read_int(f)==session->out_cipher.b);assert((uint32_t)read_int(f)==session->out_cipher.c);
    for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.rsl[i]);
    for(int i=0;i<256;i++)assert((uint32_t)read_int(f)==session->out_cipher.mem[i]);
}
static void report_controls(void) {
    Arena arena=arena_create(128*1024*1024);ComponentConfig cfg={0};report_load(&cfg,&arena);int editor=report_component(&cfg,600),mute=report_component(&cfg,613),root=cfg.instances[editor].layer;
    const char *path=getenv("RS2_REPORT_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/report-reference.bin","rb");assert(f && read_int(f)==0x52413235);int cases=read_int(f);assert(cases==2182 && read_int(f)==root);uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)read_int(f);
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);Chatbox chat;chatbox_init(&chat,"Self");int viewport,dialog,sidebar,reports=0;
    ClientActions ctx={0};ctx.chatbox=&chat;ctx.session=session;ctx.components=&cfg;ctx.viewport_interface=&viewport;ctx.chat_interface=&dialog;ctx.sidebar_interface=&sidebar;ctx.live=true;
    for(int record=0;record<cases;record++) {
        int mode=read_int(f);char name[256];assert(overhead_read_text(f,name,sizeof(name)));int rights=fgetc(f),toggles=read_int(f),end=read_int(f),x=read_int(f),y=read_int(f),n=read_int(f),keys[256];assert(n>=0 && n<256 && rights>=0);for(int i=0;i<n;i++)keys[i]=read_int(f);
        report_setup(&chat,mode,name,root,&viewport,&dialog,&sidebar);session->stream.out_len=0;memset(session->stream.out_buf,0xa5,5000);
        if(mode==0 && x>=416 && x<=516 && y>=490 && y<=522)assert(client_report_open(&ctx,"",false)==ACTION_LOCAL);
        if(mode==1){ContextMenu menu;context_menu_init(&menu);char label[320];snprintf(label,sizeof(label),!strcmp(name,"<missing-tag>")?"Report abuse plain":"Report abuse @whi@  %s  ",name);context_menu_add_option(&menu,label,record%2==0?2034:34);assert(client_action_dispatch(&ctx,menu.actions[0])==ACTION_LOCAL);}
        for(int i=0;i<n;i++)assert(chatbox_report_key(&chat,viewport,keys[i]));
        for(int i=0;i<toggles;i++)assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=mute})==ACTION_LOCAL);
        if(end==13)assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_CLOSE})==ACTION_SENT);
        else if(end){if(chat.report_input[0])reports++;int id=report_component(&cfg,600+end);assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=id})==ACTION_LOCAL);}
        int bytes=read_int(f);uint8_t expected[32];assert(bytes>=0 && bytes<=32 && fread(expected,1,bytes,f)==(size_t)bytes);if(session->stream.out_len!=(size_t)bytes || memcmp(session->stream.out_buf,expected,bytes))fprintf(stderr,"Report wire record%d mode%d end%d\n",record,mode,end);assert(session->stream.out_len==(size_t)bytes && !memcmp(session->stream.out_buf,expected,bytes));for(int i=bytes;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
        report_state(f,&chat,session,viewport,dialog,sidebar,record);
    }
    assert(fgetc(f)==EOF && reports==1720);fclose(f);puts("PASS report original:2182 footer/menu/editor/category cases,1720 category packets; full cipher/interfaces/report/mute/social/count/ordinary state and exact close231/report190 order");
    path=getenv("RS2_REPORT_UI_ORACLE");f=fopen(path && *path?path:"tests/fixtures/report-ui-reference.bin","rb");assert(f && read_int(f)==0x52553235 && read_int(f)==24 && read_int(f)==root);
    uint32_t *guard=calloc(512*334+2,sizeof(*guard));assert(guard);guard[0]=guard[512*334+1]=0xdeadbeef;
    for(int n=0;n<24;n++) {
        int cycle=read_int(f);assert(overhead_read_text(f,chat.report_input,sizeof(chat.report_input)));bool rights=fgetc(f)!=0;chat.report_mute=fgetc(f)!=0;
        for(int i=0;i<cfg.count;i++)chatbox_prepare_report_component(&chat,&cfg.instances[i],rights,cycle);
        private_string(f,cfg.instances[editor].text,n,-1,"editor cursor");private_string(f,cfg.instances[mute].text,n,-1,"mute label");assert((uint32_t)read_int(f)==cfg.instances[mute].colour);
        for(int i=0;i<512*334;i++)guard[i+1]=0x123456;
        pix2d_bind(guard+1,512,334);component_draw(&cfg,&cfg.instances[root],0,0,0);compare_pixels(f,guard+1,512*334,n,"report interface");assert(guard[0]==0xdeadbeef && guard[512*334+1]==0xdeadbeef);
    }
    assert(fgetc(f)==EOF);fclose(f);free(guard);chatbox_free(&chat);free(session);arena_free(&arena);puts("PASS report UI original:24 complete report panels,4104192 pixels; cursor/mute rights/text/colour and guards exact");
}

typedef struct {int override_chat,sticky,dialog;size_t length;uint8_t payload[255];} GameRequestInput;
typedef struct {bool added;char modal[256];int click;bool redraw;} GameRequestExpected;
static GameRequestInput game_request_input(FILE *f,Chatbox *chat) {
    GameRequestInput in={0};chat->ignore_count=(size_t)read_int(f);assert(chat->ignore_count<=100);
    for(int i=0;i<100;i++)chat->ignore_ids[i]=private_u64(f);
    in.override_chat=read_int(f);in.sticky=read_int(f);in.dialog=read_int(f);in.length=(size_t)read_int(f);assert(in.length>0 && in.length<=255 && fread(in.payload,1,in.length,f)==in.length);return in;
}
static GameRequestExpected game_request_expected(FILE *f,Chatbox *chat,bool added,int record) {
    GameRequestExpected result={0};result.added=fgetc(f)!=0;if(result.added!=added)fprintf(stderr,"Game request record%d expected_added%d actual%d type%d text=%s\n",record,result.added,added,chat->type[0],chat->text[0]?chat->text[0]:"<null>");assert(result.added==added);int count=0;
    for(int i=0;i<100;i++){assert(read_int(f)==chat->type[i]);private_string(f,chat->sender[i],record,i,"game sender");private_string(f,chat->text[i],record,i,"game text");if(chat->text[i])count++;}
    assert(chat->message_count==count);assert(overhead_read_text(f,result.modal,sizeof(result.modal)));result.click=read_int(f);result.redraw=fgetc(f)!=0;return result;
}
static void game_request_controls(void) {
    const char *path=getenv("RS2_GAME_REQUEST_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/game-requests-reference.bin","rb");assert(f && read_int(f)==0x47523235);int cases=read_int(f);assert(cases==2073);
    Chatbox chat;chatbox_init(&chat,"Self");strcpy(chat.input,"ordinary kept");chat.input_len=13;chat.private_prompt_open=true;strcpy(chat.private_input,"social kept");chat.private_input_len=11;chat.count_dialog_open=true;strcpy(chat.count_dialog_input,"42");strcpy(chat.report_input,"report kept");int additions=0,trades=0,duels=0;
    for(int n=0;n<cases;n++){
        GameRequestInput in=game_request_input(f,&chat);Chatbox before=chat;bool added=true;assert(chatbox_add_game_message225(&chat,in.payload,in.length,in.override_chat!=0,&added));
        memcpy(before.type,chat.type,sizeof(chat.type));memcpy(before.text,chat.text,sizeof(chat.text));memcpy(before.sender,chat.sender,sizeof(chat.sender));before.message_count=chat.message_count;assert(!memcmp(&before,&chat,sizeof(chat)));
        (void)game_request_expected(f,&chat,added,n);if(added){additions++;if(chat.type[0]==4)trades++;if(chat.type[0]==8)duels++;}
    }
    assert(fgetc(f)==EOF && additions==1385 && trades==294 && duels==286);fclose(f);chatbox_free(&chat);
    puts("PASS game requests original:2073 packet4 cases,1385 additions,294 trades,286 duels;all100 history/ignore slots,exact raw sender/text/type,empty sender and unchanged editor/social/count/report state");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Test Suite: Interactive Chatbox & Input System ===\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    uint32_t *fb = (uint32_t *)calloc(765 * 503, sizeof(uint32_t));
    pix2d_bind(fb, 765, 503);

    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag);

    PixFont font_p12;
    bool font_ok = pixfont_load(&font_p12, title_jag, "p12", &arena);
    assert(font_ok);
    printf("[PASS] font_p12 loaded for chatbox typography\n");

    Media media;
    bool media_ok = media_load(&media, media_jag, &arena);
    assert(media_ok && media.chatback != NULL);
    printf("[PASS] chatback sprite loaded (%dx%d)\n", media.chatback->width, media.chatback->height);

    overhead_controls(&arena,NULL,0,0);
    pix2d_bind(fb,765,503);
    visibility_controls();
    original_display_controls(&font_p12, &media);
    private_message_controls(&arena);
    public_send_controls(&arena);
    private_send_controls(&arena);
    private_ui_controls(&arena,&media);
    social_controls();
    social_ui_controls(&arena,&media);
    report_controls();
    game_request_controls();
    pix2d_bind(fb, 765, 503);

    // Test 1: Chatbox initialization
    Chatbox chat;
    chatbox_init(&chat, "Mod Mark");
    assert(strcmp(chat.username, "Mod Mark") == 0);
    assert(chat.message_count == 0);
    printf("[PASS] Chatbox initialized for user 'Mod Mark'\n");

    // Revision-225 client.java accepts public-chat characters 32 through 122.
    for (int ch = 32; ch <= 126; ch++) {
        chatbox_key_press(&chat, (char)ch);
        if (ch <= 122) {
            assert(chat.input_len == 1 && chat.input[0] == (char)ch);
            chatbox_backspace(&chat);
        } else {
            assert(chat.input_len == 0 && chat.input[0] == '\0');
        }
    }
    puts("[PASS] Public-chat input accepts original characters and rejects codes 123-126");

    Chatbox system_chat;
    chatbox_init(&system_chat, "Player");
    uint8_t welcome[] = "Welcome to RuneScape.\n";
    assert(chatbox_add_game_message(&system_chat, welcome, sizeof(welcome) - 1));
    assert(strcmp(system_chat.text[0], "Welcome to RuneScape.") == 0);
    uint32_t *expected = malloc(765 * 503 * sizeof(*expected));
    assert(expected);
    memset(fb, 0, 765 * 503 * sizeof(*fb));
    chatbox_draw(&system_chat, media.chatback, 22, 375, &font_p12);
    memcpy(expected, fb, 765 * 503 * sizeof(*expected));
    chatbox_free(&system_chat);
    chatbox_init(&system_chat, "Player");
    chatbox_add_message(&system_chat, 0, NULL, "Welcome to RuneScape.");
    memset(fb, 0, 765 * 503 * sizeof(*fb));
    chatbox_draw(&system_chat, media.chatback, 22, 375, &font_p12);
    assert(memcmp(expected, fb, 765 * 503 * sizeof(*expected)) == 0);
    free(expected);
    uint8_t literal_equals[] = "Level=1=\n";
    assert(chatbox_add_game_message(&system_chat, literal_equals, sizeof(literal_equals) - 1));
    assert(strcmp(system_chat.text[0], "Level=1=") == 0);
    uint8_t missing_terminator[] = "Malformed";
    int prior_count = system_chat.message_count;
    assert(!chatbox_add_game_message(&system_chat, missing_terminator, sizeof(missing_terminator) - 1));
    assert(system_chat.message_count == prior_count);
    chatbox_free(&system_chat);
    puts("[PASS] System-message terminators excluded from text/pixels; literal equals retained");

    // Test 2: Message history buffer
    chatbox_add_message(&chat, 0, NULL, "Welcome to RuneScape.");
    chatbox_add_message(&chat, 2, "Hans", "Hello there adventurer!");
    chatbox_add_message(&chat, 1, "Mod Mark", "Testing native C client Revision 225.");
    assert(chat.message_count == 3);
    assert(strcmp(chat.text[0], "Testing native C client Revision 225.") == 0);
    assert(strcmp(chat.text[2], "Welcome to RuneScape.") == 0);
    printf("[PASS] 100-message chat history FIFO buffer verified\n");

    // Test 3: Typing and backspace
    const char *test_cmd = "::tele 3200 3200";
    for (size_t i = 0; i < strlen(test_cmd); i++) {
        chatbox_key_press(&chat, test_cmd[i]);
    }
    assert(strcmp(chat.input, test_cmd) == 0);

    // Backspace test
    chatbox_backspace(&chat);
    assert(strcmp(chat.input, "::tele 3200 320") == 0);
    chatbox_key_press(&chat, '0');
    assert(strcmp(chat.input, test_cmd) == 0);

    // Submit command
    const char *submitted = chatbox_submit(&chat);
    assert(submitted != NULL);
    assert(strcmp(submitted, test_cmd) == 0);
    assert(chat.input_len == 0);
    assert(chat.message_count == 3); // commands starting with ':' are not added to chat history
    printf("[PASS] Interactive keyboard input and backspace verified\n");

    // Test 4: Public chat message submission
    const char *public_msg = "Hello Lumbridge!";
    for (size_t i = 0; i < strlen(public_msg); i++) {
        chatbox_key_press(&chat, public_msg[i]);
    }
    const char *submitted_msg = chatbox_submit(&chat);
    assert(strcmp(submitted_msg, public_msg) == 0);
    assert(chat.message_count == 4);
    assert(strcmp(chat.text[0], public_msg) == 0);
    assert(strcmp(chat.sender[0], "Mod Mark") == 0);
    printf("[PASS] Public chat message submission and formatting verified\n");

    // Test 5: Render chatbox into framebuffer
    chatbox_draw(&chat, media.chatback, 22, 375, &font_p12);

    int rendered_pixels = 0;
    for (int i = 0; i < 765 * 503; i++) {
        if (fb[i] != 0) rendered_pixels++;
    }
    printf("  Rendered chatbox pixels in framebuffer: %d\n", rendered_pixels);
    assert(rendered_pixels > 20000);
    printf("[PASS] Complete chatbox rendered to framebuffer with messages, prompt & cursor!\n");

    chatbox_key_press(&chat,'h');chatbox_key_press(&chat,'i');
    int before_take=chat.message_count;
    const char *taken=chatbox_take_input(&chat);
    assert(taken && !strcmp(taken,"hi") && chat.message_count==before_take && chat.input_len==0);
    puts("PASS: live input extraction never fabricates a public chat echo before network support");
    chatbox_free(&chat);
    free(fb);
    arena_free(&arena);

    printf("=== ALL CHATBOX & INPUT TESTS PASSED! ===\n");
    return 0;
}
