#include "config/flo.h"
#include "config/seq.h"
#include "config/loc.h"
#include "config/npc.h"
#include "config/obj.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("=== Test Suite: Cache Configs (Rev 225) ===\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    const char* config_path = "../engine/data/pack/client/config";
    Jagfile* jag = jagfile_load_file(config_path, &arena);
    if (!jag) {
        config_path = "engine/data/pack/client/config";
        jag = jagfile_load_file(config_path, &arena);
    }
    assert(jag != NULL);
    printf("[PASS] Config Jagfile archive loaded (%d files)\n", jag->file_count);

    // 1. Test FloType
    FloConfig flo_cfg;
    flo_unpack(&flo_cfg, jag);
    printf("[INFO] Flo count: %d\n", flo_cfg.count);
    assert(flo_cfg.count > 0);
    printf("[PASS] FloType unpack verified (Flo 0: rgb=0x%06X, hsl=%d)\n", flo_cfg.instances[0].rgb, flo_cfg.instances[0].hsl);

    // 2. Test SeqType
    SeqConfig seq_cfg;
    seq_unpack(&seq_cfg, jag);
    printf("[INFO] Seq count: %d\n", seq_cfg.count);
    assert(seq_cfg.count > 0);
    assert(seq_cfg.instances[0].frame_count >= 1);
    printf("[PASS] SeqType unpack verified (Seq 0: frame_count=%d)\n", seq_cfg.instances[0].frame_count);

    // 3. Test LocType
    LocConfig loc_cfg;
    loc_unpack(&loc_cfg, jag);
    printf("[INFO] Loc count: %d\n", loc_cfg.count);
    assert(loc_cfg.count > 0);

    LocType loc;
    bool loc_found = false;
    for (int i = 0; i < loc_cfg.count && i < 2000; i++) {
        if (loc_get(&loc_cfg, i, &loc)) {
            if (strlen(loc.name) > 0) {
                printf("[INFO] Sample Loc %d: '%s' (width=%d, length=%d, active=%d)\n", i, loc.name, loc.width, loc.length, loc.active);
                loc_found = true;
                loc_free_entry(&loc);
                break;
            }
            loc_free_entry(&loc);
        }
    }
    assert(loc_found);
    printf("[PASS] LocType index & stream decode verified\n");

    // 4. Test NpcType
    NpcConfig npc_cfg;
    npc_unpack(&npc_cfg, jag);
    printf("[INFO] Npc count: %d\n", npc_cfg.count);
    assert(npc_cfg.count > 0);

    NpcType npc;
    bool npc0_ok = npc_get(&npc_cfg, 0, &npc);
    assert(npc0_ok);
    printf("[INFO] NPC 0: '%s' (vislevel=%d, size=%d)\n", npc.name, npc.vislevel, npc.size);
    assert(strcmp(npc.name, "Hans") == 0);
    npc_free_entry(&npc);
    printf("[PASS] NpcType verified NPC 0 is 'Hans'\n");

    // 5. Test ObjType
    ObjConfig obj_cfg;
    obj_unpack(&obj_cfg, jag, true);
    printf("[INFO] Obj count: %d\n", obj_cfg.count);
    assert(obj_cfg.count > 0);

    // Check Item 1277 (Bronze sword)
    ObjType sword;
    bool obj_sword = obj_get(&obj_cfg, 1277, &sword);
    assert(obj_sword);
    printf("[INFO] Item 1277: '%s' (cost=%d, members=%d, model=%d)\n", sword.name, sword.cost, sword.members, sword.model);
    assert(strcmp(sword.name, "Bronze sword") == 0);
    obj_free_entry(&sword);

    // Check Item 995 (Coins)
    ObjType coins;
    bool obj_coins = obj_get(&obj_cfg, 995, &coins);
    assert(obj_coins);
    printf("[INFO] Item 995: '%s' (stackable=%d)\n", coins.name, coins.stackable);
    assert(strcmp(coins.name, "Coins") == 0);
    assert(coins.stackable == true);
    obj_free_entry(&coins);

    printf("[PASS] ObjType verified Item 1277 ('Bronze sword') and Item 995 ('Coins')\n");

    // Clean up
    flo_free(&flo_cfg);
    seq_free(&seq_cfg);
    loc_free(&loc_cfg);
    npc_free(&npc_cfg);
    obj_free(&obj_cfg);
    arena_free(&arena);

    printf("=== ALL CONFIG TESTS PASSED! ===\n");
    return 0;
}
