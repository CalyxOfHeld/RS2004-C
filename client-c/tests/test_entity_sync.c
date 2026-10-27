#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "core/packet.h"
#include "core/jstring.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include "config/flo.h"
#include "config/seq.h"
#include "config/loc.h"
#include "config/npc.h"
#include "config/obj.h"
#include "config/idk.h"
#include "entity/pathing_entity.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "net/entity_sync.h"
#include "ui/chatbox.h"

#include "../verification/completion/tools/reset_multiway_reference_control.h"

static void put_bits(uint8_t *data,int *bit,int value,int count) {
    for(int b=count-1;b>=0;b--,(*bit)++) if((value>>b)&1) data[*bit/8]|=1<<(7-(*bit&7));
}
static int pub_int(FILE *f) {uint32_t v=0;for(int i=0;i<4;i++){int ch=fgetc(f);assert(ch>=0);v=v*256+(unsigned)ch;}return v<=INT32_MAX?(int)v:-(int)(UINT32_MAX-v)-1;}
static void pub_text(FILE *f,char *out,size_t capacity,bool *is_null) {
    int n=pub_int(f);*is_null=n==-1;assert(n>=-1 && (n<0 || (size_t)n<capacity));
    if(n<0){out[0]=0;return;}for(int i=0;i<n;i++){int hi=fgetc(f),lo=fgetc(f);assert(hi==0 && lo>=0);out[i]=(char)lo;}out[n]=0;
}
typedef struct {Chatbox *chat;WordFilterTables *filter;bool override;} PublicControl;
static void public_callback(void *user,PlayerEntity *player,bool packed,int colour,int rights,const uint8_t *text,size_t length) {
    PublicControl *control=user;
    if(packed)(void)chatbox_add_public_message(control->chat,control->filter,player,text,length,colour,rights,control->override);
    else chatbox_add_message(control->chat,2,player->name,(const char*)text);
}
static void public_chat_controls(Arena *arena) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",arena);assert(jag);WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    const char *path=getenv("RS2_PUBLIC_CHAT_ORACLE");FILE *f=fopen(path && *path?path:"tests/fixtures/public-chat-reference.bin","rb");assert(f);
    assert(pub_int(f)==0x50433235);int count=pub_int(f);assert(count==3237);
    Chatbox chat;chatbox_init(&chat,"Self");PublicControl control={&chat,&tables,false};
    EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(ctx);entity_sync_init(ctx,NULL);ctx->player_chat=public_callback;ctx->player_chat_user=&control;ctx->cycle=1000;
    int additions=0;
    for(int record=0;record<count;record++) {
        PlayerEntity *player=&ctx->local_player;player_entity_init(player);bool null_name=false;pub_text(f,player->name,sizeof(player->name),&null_name);
        int ignored=fgetc(f),override=pub_int(f),mask=pub_int(f),length=pub_int(f);assert(ignored>=0 && length>=0 && length<512);
        uint8_t raw[512];assert(fread(raw,1,length,f)==(size_t)length);chat.ignore_count=ignored?1:0;chat.ignore_ids[0]=jstring_to_base37(null_name?"alice":player->name);control.override=override!=0;
        strcpy(player->base.chat,"old");player->base.chat_color=13;player->base.chat_style=17;player->base.chat_timer=7;
        uint8_t bytes[1024]={0};int bits=0;put_bits(bytes,&bits,1,1);put_bits(bytes,&bits,0,2);put_bits(bytes,&bits,0,8);put_bits(bytes,&bits,2047,11);
        Packet packet=packet_wrap(bytes,sizeof(bytes));packet.pos=(bits+7)/8;p1(&packet,mask>255?(mask&255)|128:mask);if(mask>255)p1(&packet,mask>>8);
        size_t start=packet.pos;
        memcpy(bytes+packet.pos,raw,length);packet.pos+=(size_t)length;size_t size=packet.pos;packet.pos=0;char *previous=chat.text[0];
        bool ok=entity_sync_player_info(ctx,&packet,size);if(!ok)fprintf(stderr,"Public mask record%d failed pos%zu size%zu\n",record,packet.pos,size);assert(ok);
        if(chat.text[0]!=previous)additions++;
        assert((int)(packet.pos-start)==pub_int(f));char expected[512];bool is_null;pub_text(f,expected,sizeof(expected),&is_null);
        if(strcmp(expected,player->base.chat))fprintf(stderr,"Public actor mismatch record%d expected[%s] actual[%s]\n",record,expected,player->base.chat);
        assert(!strcmp(expected,player->base.chat));assert(pub_int(f)==player->base.chat_color);assert(pub_int(f)==player->base.chat_style);assert(pub_int(f)==player->base.chat_timer);
        assert(pub_int(f)==player->base.spotanim_id);assert(pub_int(f)==player->base.spotanim_height);assert(pub_int(f)==player->base.spotanim_delay);assert(pub_int(f)==player->base.spotanim_frame);assert(pub_int(f)==player->base.spotanim_cycle);
        int messages=0;
        for(int i=0;i<100;i++){
            assert(pub_int(f)==chat.type[i]);pub_text(f,expected,sizeof(expected),&is_null);assert(is_null?!chat.sender[i]:(chat.sender[i] && !strcmp(expected,chat.sender[i])));
            pub_text(f,expected,sizeof(expected),&is_null);assert(is_null?!chat.text[i]:(chat.text[i] && !strcmp(expected,chat.text[i])));if(chat.text[i])messages++;
        }
        assert(messages==chat.message_count);
    }
    assert(additions==2586);
    ctx->players[3]=calloc(1,sizeof(PlayerEntity));ctx->players[4]=calloc(1,sizeof(PlayerEntity));ctx->npcs[2]=calloc(1,sizeof(NpcEntity));assert(ctx->players[3] && ctx->players[4] && ctx->npcs[2]);
    ctx->player_indices[0]=3;ctx->player_count=1;ctx->npc_indices[0]=2;ctx->npc_count=1;
    PathingEntity *actors[]={&ctx->local_player.base,&ctx->players[3]->base,&ctx->players[4]->base,&ctx->npcs[2]->base};int timers[]={150,1,19,100};
    for(int i=0;i<4;i++){strcpy(actors[i]->chat,"text");actors[i]->chat_timer=timers[i];}
    int ticks=pub_int(f);assert(ticks==155);
    for(int i=0;i<ticks;i++){entity_sync_tick_chats(ctx);for(int a=0;a<4;a++){assert(pub_int(f)==actors[a]->chat_timer);int nil=fgetc(f);assert(nil>=0 && (nil!=0)==(!actors[a]->chat[0]));}}
    assert(fgetc(f)==EOF);fclose(f);
    /* Declared public payload must be bounded before any chat callback; later
     * masks remain aligned even when decoder/filter fails (corpus above). */
    ctx->player_count=0;ctx->npc_count=0;strcpy(ctx->local_player.name,"Alice");
    uint8_t short_data[16]={0};int bits=0;put_bits(short_data,&bits,1,1);put_bits(short_data,&bits,0,2);put_bits(short_data,&bits,0,8);put_bits(short_data,&bits,2047,11);
    Packet bad=packet_wrap(short_data,sizeof(short_data));bad.pos=(bits+7)/8;p1(&bad,64);p2(&bad,0x1234);p1(&bad,2);p1(&bad,4);p1(&bad,0);
    size_t size=bad.pos;bad.pos=0;Chatbox before=chat;PathingEntity actor_before=ctx->local_player.base;
    assert(!entity_sync_player_info(ctx,&bad,size));assert(!memcmp(&chat,&before,sizeof(chat)));assert(!strcmp(ctx->local_player.base.chat,actor_before.chat) && ctx->local_player.base.chat_timer==actor_before.chat_timer);
    assert(!chatbox_add_public_message(&chat,&tables,&ctx->local_player,NULL,1,0,0,false));
    assert(!chatbox_add_public_message(&chat,NULL,&ctx->local_player,short_data,1,0,2,false));assert(!memcmp(&chat,&before,sizeof(chat)));
    entity_sync_cleanup(ctx);free(ctx);chatbox_free(&chat);wordfilter_tables_free(&tables);
    printf("PASS public/forced original masks: %d actor/history/position states,2586 history-changing steps;155 expiry ticks/four actors; suppression/decode failure/suffix alignment/malformed/filter-required controls\n",count);
}
int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Entity System & Synchronization ===\n");

    // 1. Test Base37 & JString
    uint64_t b37 = jstring_to_base37("antigravity");
    char name_buf[64];
    jstring_from_base37(b37, name_buf, sizeof(name_buf));
    jstring_format_name(name_buf);
    printf("[INFO] Base37 encode/decode: 'antigravity' -> 0x%llX -> '%s'\n", (unsigned long long)b37, name_buf);
    assert(strcmp(name_buf, "Antigravity") == 0);
    printf("[PASS] JString Base37 conversions verified\n");

    // 2. Load Configs & Jagfiles
    Arena arena = arena_create(32 * 1024 * 1024);
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) {
        config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    }
    assert(config_jag != NULL);

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models_jag) {
        models_jag = jagfile_load_file("engine/data/pack/client/models", &arena);
    }
    assert(models_jag != NULL);

    model_init_repo(models_jag, &arena);

    NpcConfig npc_cfg;
    npc_unpack(&npc_cfg, config_jag);
    assert(npc_cfg.count > 0);

    ObjConfig obj_cfg;
    obj_unpack(&obj_cfg, config_jag, false);
    assert(obj_cfg.count > 0);

    IdkConfig idk_cfg;
    idk_unpack(&idk_cfg, config_jag, &arena);
    assert(idk_cfg.count > 0);
    printf("[PASS] IdkConfig unpacked (%d identity kits)\n", idk_cfg.count);

    // 3. Test NPC Entity & Model Generation
    NpcEntity hans;
    npc_entity_init(&hans);
    bool got_hans = npc_get(&npc_cfg, 0, &hans.type);
    assert(got_hans);
    hans.has_type = true;
    printf("[INFO] NPC 0 is '%s' (combat=%d, size=%d)\n", hans.type.name, hans.type.vislevel, hans.type.size);
    assert(strcmp(hans.type.name, "Hans") == 0);

    Model *hans_model = npc_entity_get_model(&hans, &arena);
    assert(hans_model != NULL);
    printf("[INFO] Hans model generated: %d points, %d faces\n", hans_model->num_points, hans_model->num_faces);
    assert(hans_model->num_points > 50);
    assert(hans_model->num_faces > 50);
    printf("[PASS] NPC entity model generation verified\n");

    // 4. Test Player Appearance Decoding & Model Generation
    PlayerEntity player;
    player_entity_init(&player);

    // Build a simulated Rev 225 appearance block:
    // gender=0 (male), headicons=0
    // appearances: head=256+0, back=0, neck=0, weapon=0, body=256+18, shield=0, arms=256+26, legs=256+36, hair=0, hands=256+33, boots=256+42, beard=256+10
    // colors: hair=0, torso=2, legs=2, feet=0, skin=0
    // seqs: stand=808, turn=823, walk=819, turn_around=820, turn_left=821, turn_right=822, run=824
    // name="antigravity", combat=3
    uint8_t app_raw[128];
    Packet app_pkt = packet_wrap(app_raw, sizeof(app_raw));
    p1(&app_pkt, 0); // gender male
    p1(&app_pkt, 0); // headicons

    int default_app[12] = { 256 + 0, 0, 0, 0, 256 + 18, 0, 256 + 26, 256 + 36, 0, 256 + 33, 256 + 42, 256 + 10 };
    for (int i = 0; i < 12; i++) {
        if (default_app[i] == 0) {
            p1(&app_pkt, 0);
        } else {
            p1(&app_pkt, (default_app[i] >> 8) & 0xFF);
            p1(&app_pkt, default_app[i] & 0xFF);
        }
    }
    p1(&app_pkt, 0); // hair color
    p1(&app_pkt, 2); // torso color
    p1(&app_pkt, 2); // legs color
    p1(&app_pkt, 0); // feet color
    p1(&app_pkt, 0); // skin color

    p2(&app_pkt, 808); // stand
    p2(&app_pkt, 823); // turn
    p2(&app_pkt, 819); // walk
    p2(&app_pkt, 820); // turn_around
    p2(&app_pkt, 821); // turn_left
    p2(&app_pkt, 822); // turn_right
    p2(&app_pkt, 824); // run

    p8(&app_pkt, (int64_t)jstring_to_base37("antigravity"));
    p1(&app_pkt, 3); // combat level 3

    size_t app_len = app_pkt.pos;
    app_pkt.pos = 0;
    player_entity_read_appearance(&player, &app_pkt);

    assert(player.gender == 0);
    assert(strcmp(player.name, "Antigravity") == 0);
    assert(player.combat_level == 3);
    assert(player.base.seq_stand_id == 808);
    printf("[PASS] Player appearance block parsed: name='%s', combat=%d, stand_seq=%d\n",
           player.name, player.combat_level, player.base.seq_stand_id);

    Model *player_model = player_entity_get_model(&player, &idk_cfg, &obj_cfg, NULL, &arena);
    assert(player_model != NULL);
    printf("[INFO] Player composite 3D model generated: %d points, %d faces\n",
           player_model->num_points, player_model->num_faces);
    assert(player_model->num_points > 100);
    assert(player_model->num_faces > 100);
    printf("[PASS] Player full composite 3D model successfully assembled and lit\n");

    // 5. Test Live EntitySyncContext (Packet 184 & Packet 1 Bitstream Parsing)
    EntitySyncContext sync_ctx;
    entity_sync_init(&sync_ctx, &npc_cfg);

    // Synthesize Packet 184 (PLAYER_INFO)
    uint8_t pinfo_buf[512];
    Packet pinfo_pkt = packet_wrap(pinfo_buf, sizeof(pinfo_buf));

    // Bitstream:
    // Local player: req=1, type=3 (teleport), plane=0, x=32, z=48, clear_walk=1, ext=1
    // Old players: count=0
    // New players: slot=15, dx=2, dz=-1, clear_walk=1, ext=0
    // Terminator: slot=2047
    access_bits(&pinfo_pkt);
    // Local player:
    // We can simulate bits directly using helper
    // 1 bit req=1
    // 2 bit type=3
    // 2 bit plane=0
    // 7 bit x=32
    // 7 bit z=48
    // 1 bit clear_walk=1
    // 1 bit ext=1
    // Old players: 8 bit count=0
    // New players: 11 bit slot=15, 5 bit dx=2, 5 bit dz=31 (-1), 1 bit clear=1, 1 bit ext=0
    // Terminator: 11 bit slot=2047
    // Let's write the bitstream manually into bytes:
    // Total bits = 1 + 2 + 2 + 7 + 7 + 1 + 1 + 8 + 11 + 5 + 5 + 1 + 1 + 11 = 63 bits (~8 bytes)
    // To write bitstream cleanly, let's use Packet write bitstream if available or pack bits:
    uint8_t bit_buffer[16] = {0};
    int bpos = 0;
    #define WRITE_BITS(val, n) do { \
        for (int b = (n) - 1; b >= 0; b--) { \
            if (((val) >> b) & 1) { \
                bit_buffer[bpos >> 3] |= (1 << (7 - (bpos & 7))); \
            } \
            bpos++; \
        } \
    } while(0)

    WRITE_BITS(1, 1);  // local_update_req
    WRITE_BITS(3, 2);  // update_type = teleport
    WRITE_BITS(0, 2);  // plane = 0
    WRITE_BITS(32, 7); // tile_x = 32
    WRITE_BITS(48, 7); // tile_z = 48
    WRITE_BITS(1, 1);  // clear_walk
    WRITE_BITS(1, 1);  // ext = 1

    WRITE_BITS(0, 8);  // old_players = 0

    WRITE_BITS(15, 11); // new player slot 15
    WRITE_BITS(2, 5);   // dx = +2
    WRITE_BITS(31, 5);  // dz = -1 (31 in 5-bit unsigned)
    WRITE_BITS(1, 1);   // clear_walk
    WRITE_BITS(0, 1);   // ext = 0

    WRITE_BITS(2047, 11); // terminator slot 2047
    #undef WRITE_BITS

    int byte_len = (bpos + 7) / 8;
    for (int i = 0; i < byte_len; i++) {
        p1(&pinfo_pkt, bit_buffer[i]);
    }

    // Extended update block for local player:
    // mask = 0x1 (appearance block)
    p1(&pinfo_pkt, 0x1);
    p1(&pinfo_pkt, (int32_t)app_len);
    for (size_t i = 0; i < app_len; i++) {
        p1(&pinfo_pkt, app_raw[i]);
    }

    size_t total_pinfo_size = pinfo_pkt.pos;
    pinfo_pkt.pos = 0;

    assert(entity_sync_player_info(&sync_ctx, &pinfo_pkt, total_pinfo_size));

    assert(sync_ctx.local_player.base.path_tile_x[0] == 32);
    assert(sync_ctx.local_player.base.path_tile_z[0] == 48);
    assert(strcmp(sync_ctx.local_player.name, "Antigravity") == 0);
    assert(sync_ctx.player_count == 1);
    assert(sync_ctx.players[15] != NULL);
    assert(sync_ctx.players[15]->base.path_tile_x[0] == 34); // 32 + 2
    assert(sync_ctx.players[15]->base.path_tile_z[0] == 47); // 48 - 1
    printf("[PASS] Packet 184 (PLAYER_INFO) bitstream synchronization verified! (Local & other players)\n");

    // 6. Test NPC_INFO (Packet 1)
    uint8_t npc_buf[256];
    Packet npc_pkt = packet_wrap(npc_buf, sizeof(npc_buf));

    uint8_t npc_bits[16] = {0};
    bpos = 0;
    #define WRITE_BITS(val, n) do { \
        for (int b = (n) - 1; b >= 0; b--) { \
            if (((val) >> b) & 1) { \
                npc_bits[bpos >> 3] |= (1 << (7 - (bpos & 7))); \
            } \
            bpos++; \
        } \
    } while(0)

    WRITE_BITS(0, 8);   // old_npcs = 0
    WRITE_BITS(5, 13);  // new npc slot 5
    WRITE_BITS(0, 11);  // npc_type = 0 (Hans)
    WRITE_BITS(1, 5);   // dx = +1
    WRITE_BITS(3, 5);   // dz = +3
    WRITE_BITS(0, 1);   // ext = 0
    WRITE_BITS(8191, 13); // terminator slot 8191
    #undef WRITE_BITS

    byte_len = (bpos + 7) / 8;
    for (int i = 0; i < byte_len; i++) {
        p1(&npc_pkt, npc_bits[i]);
    }

    size_t total_npc_size = npc_pkt.pos;
    npc_pkt.pos = 0;

    assert(entity_sync_npc_info(&sync_ctx, &npc_pkt, total_npc_size));

    assert(sync_ctx.npc_count == 1);
    assert(sync_ctx.npcs[5] != NULL);
    assert(sync_ctx.npcs[5]->has_type);
    assert(strcmp(sync_ctx.npcs[5]->type.name, "Hans") == 0);
    assert(sync_ctx.npcs[5]->base.path_tile_x[0] == 33); // 32 + 1
    assert(sync_ctx.npcs[5]->base.path_tile_z[0] == 51); // 48 + 3
    printf("[PASS] Packet 1 (NPC_INFO) bitstream synchronization verified! (Hans spawned at tile (33, 51))\n");
    /* Original removal IDs are resolved after new-visible slots are decoded. */
    NpcEntity *retained_npc=sync_ctx.npcs[5];
    int retained_x=retained_npc->base.x,retained_z=retained_npc->base.z;
    uint8_t reuse_bits[32]={0};int reuse_pos=0;
    put_bits(reuse_bits,&reuse_pos,1,8); /* existing list length */
    put_bits(reuse_bits,&reuse_pos,1,1);put_bits(reuse_bits,&reuse_pos,3,2);
    put_bits(reuse_bits,&reuse_pos,5,13);put_bits(reuse_bits,&reuse_pos,0,11);
    put_bits(reuse_bits,&reuse_pos,1,5);put_bits(reuse_bits,&reuse_pos,4,5);
    put_bits(reuse_bits,&reuse_pos,0,1);put_bits(reuse_bits,&reuse_pos,8191,13);
    Packet reuse=packet_wrap(reuse_bits,(reuse_pos+7)/8);
    assert(entity_sync_npc_info(&sync_ctx,&reuse,reuse.length));
    assert(sync_ctx.npc_count==1 && sync_ctx.npcs[5]==retained_npc);
    assert(retained_npc->base.x==retained_x && retained_npc->base.z==retained_z);
    assert(retained_npc->base.path_tile_x[0]==33 && retained_npc->base.path_tile_z[0]==52);
    assert(retained_npc->base.path_length==1);
    puts("PASS: same-packet NPC removal/reappearance preserves queued movement and object identity");
    memset(reuse_bits,0,sizeof(reuse_bits));reuse_pos=0;
    put_bits(reuse_bits,&reuse_pos,1,8);put_bits(reuse_bits,&reuse_pos,1,1);put_bits(reuse_bits,&reuse_pos,3,2);
    put_bits(reuse_bits,&reuse_pos,8191,13);reuse=packet_wrap(reuse_bits,(reuse_pos+7)/8);
    assert(entity_sync_npc_info(&sync_ctx,&reuse,reuse.length));
    assert(sync_ctx.npc_count==0 && sync_ctx.npcs[5]==retained_npc);
    memset(reuse_bits,0,sizeof(reuse_bits));reuse_pos=0;
    put_bits(reuse_bits,&reuse_pos,0,8);put_bits(reuse_bits,&reuse_pos,5,13);put_bits(reuse_bits,&reuse_pos,0,11);
    put_bits(reuse_bits,&reuse_pos,1,5);put_bits(reuse_bits,&reuse_pos,5,5);
    put_bits(reuse_bits,&reuse_pos,0,1);put_bits(reuse_bits,&reuse_pos,8191,13);
    reuse=packet_wrap(reuse_bits,(reuse_pos+7)/8);assert(entity_sync_npc_info(&sync_ctx,&reuse,reuse.length));
    assert(sync_ctx.npcs[5]==retained_npc && retained_npc->base.x==retained_x && retained_npc->base.z==retained_z);
    assert(retained_npc->base.path_length==2 && retained_npc->base.path_tile_z[0]==53);
    puts("PASS: multiple NPC packets in one game cycle retain removed/reappearing actor state");



    // These actors came from the actual production bitstream decoder above.
    // A scene shift must preserve world positions and queued movement in both
    // directions, including cached players outside the current visible list.
    PathingEntity *actors[]={&sync_ctx.local_player.base,
        &sync_ctx.players[15]->base,&sync_ctx.npcs[5]->base};
    int original_x[3],original_z[3],tile_x[3][10],tile_z[3][10];
    for(int i=0;i<3;i++) {
        original_x[i]=actors[i]->x;original_z[i]=actors[i]->z;
        memcpy(tile_x[i],actors[i]->path_tile_x,sizeof(tile_x[i]));
        memcpy(tile_z[i],actors[i]->path_tile_z,sizeof(tile_z[i]));
    }
    entity_sync_rebase(&sync_ctx,16,-8);
    for(int i=0;i<3;i++) {
        assert(actors[i]->x+16*128==original_x[i]);
        assert(actors[i]->z-8*128==original_z[i]);
        for(int step=0;step<10;step++) {
            assert(actors[i]->path_tile_x[step]+16==tile_x[i][step]);
            assert(actors[i]->path_tile_z[step]-8==tile_z[i][step]);
        }
    }
    entity_sync_rebase(&sync_ctx,-16,8);
    for(int i=0;i<3;i++) {
        assert(actors[i]->x==original_x[i] && actors[i]->z==original_z[i]);
        assert(!memcmp(tile_x[i],actors[i]->path_tile_x,sizeof(tile_x[i])));
        assert(!memcmp(tile_z[i],actors[i]->path_tile_z,sizeof(tile_z[i])));
    }
    puts("PASS: decoded local/remote players and NPC retain world positions and all queued steps across reversible scene shifts");

    // Explicit wire fixtures for masks and shortened visible lists.
    uint8_t masks[128]={0};int bits=0;
    put_bits(masks,&bits,1,8);put_bits(masks,&bits,1,1);put_bits(masks,&bits,0,2);put_bits(masks,&bits,8191,13);
    Packet mask=packet_wrap(masks,sizeof(masks));mask.pos=(bits+7)/8;
    p1(&mask,0xc0);p2(&mask,45);p4(&mask,(12<<16)|7);p2(&mask,6465);p2(&mask,6467);
    size_t mask_size=mask.pos;mask.pos=0;
    assert(entity_sync_npc_info(&sync_ctx,&mask,mask_size));
    assert(sync_ctx.npcs[5]->base.spotanim_id==45 && sync_ctx.npcs[5]->base.spotanim_height==12);
    assert(sync_ctx.npcs[5]->base.target_tile_x==6465 && sync_ctx.npcs[5]->base.target_tile_z==6467);
    mask=packet_wrap(masks,mask_size-1);assert(!entity_sync_npc_info(&sync_ctx,&mask,mask_size-1));
    memset(masks,0,sizeof(masks));bits=0;
    put_bits(masks,&bits,0,8);put_bits(masks,&bits,8191,13);
    sync_ctx.cycle++; /* this removal is in a later game cycle */
    mask=packet_wrap(masks,(bits+7)/8);assert(entity_sync_npc_info(&sync_ctx,&mask,mask.length));
    assert(sync_ctx.npc_count==0 && sync_ctx.npcs[5]==NULL);
    puts("PASS: NPC face-square/spotanim mask consumption, truncation rejection and removal through shortened list");
    memset(masks,0,sizeof(masks));bits=0;
    put_bits(masks,&bits,1,1);put_bits(masks,&bits,0,2);put_bits(masks,&bits,0,8);put_bits(masks,&bits,2047,11);
    mask=packet_wrap(masks,sizeof(masks));mask.pos=(bits+7)/8;
    p1(&mask,0x80);p1(&mask,3);p2(&mask,46);p4(&mask,(13<<16)|8);
    p1(&mask,1);p1(&mask,2);p1(&mask,3);p1(&mask,4);p2(&mask,20);p2(&mask,30);p1(&mask,2);
    mask_size=mask.pos;mask.pos=0;assert(entity_sync_player_info(&sync_ctx,&mask,mask_size));
    assert(sync_ctx.local_player.base.spotanim_id==46 && sync_ctx.local_player.base.spotanim_height==13);
    assert(sync_ctx.local_player.base.exact_start_x==1 && sync_ctx.local_player.base.exact_end_z==4);
    assert(sync_ctx.player_count==0 && sync_ctx.players[15]!=NULL && !sync_ctx.players[15]->visible);
    mask=packet_wrap(masks,mask_size-1);assert(!entity_sync_player_info(&sync_ctx,&mask,mask_size-1));
    uint8_t impossible[]={1,255,255};mask=packet_wrap(impossible,sizeof(impossible));
    assert(!entity_sync_npc_info(&sync_ctx,&mask,sizeof(impossible)));
    puts("PASS: player extended high masks, cached removed appearance, malformed mask and old-count rejection (decoding only)");

    public_chat_controls(&arena);
    Media reset_media={0};Jagfile *reset_media_jag=jagfile_load_file("../engine/data/pack/client/media",&arena);assert(reset_media_jag && media_load(&reset_media,reset_media_jag,&arena));
    const char *reset_reference=getenv("RS2_RESET_ORACLE"),*multiway_reference=getenv("RS2_MULTIWAY_ORACLE");
    reset_multiway_reference_run(reset_reference && *reset_reference?reset_reference:"tests/fixtures/reset-anims-reference.bin",multiway_reference && *multiway_reference?multiway_reference:"tests/fixtures/multiway-reference.bin",&sync_ctx,&reset_media,NULL);
    entity_sync_cleanup(&sync_ctx);
    npc_entity_free(&hans);
    idk_free(&idk_cfg);
    npc_free(&npc_cfg);
    obj_free(&obj_cfg);
    arena_free(&arena);

    printf("=== ALL ENTITY & SYNCHRONIZATION TESTS PASSED! ===\n");
    return 0;
}
