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

static void put_bits(uint8_t *data,int *bit,int value,int count) {
    for(int b=count-1;b>=0;b--,(*bit)++) if((value>>b)&1) data[*bit/8]|=1<<(7-(*bit&7));
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

    entity_sync_cleanup(&sync_ctx);
    npc_entity_free(&hans);
    idk_free(&idk_cfg);
    npc_free(&npc_cfg);
    obj_free(&obj_cfg);
    arena_free(&arena);

    printf("=== ALL ENTITY & SYNCHRONIZATION TESTS PASSED! ===\n");
    return 0;
}
