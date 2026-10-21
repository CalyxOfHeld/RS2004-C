#include "net/protocol.h"
#include "net/rsa.h"
#include "net/client_stream.h"
#include "core/packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint8_t* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

static uint32_t get_file_crc(const char* path) {
    size_t sz = 0;
    uint8_t* data = read_file(path, &sz);
    if (!data) {
        fprintf(stderr, "Failed to read pack for CRC: %s\n", path);
        return 0;
    }
    uint32_t crc = packet_crc32(data, sz);
    free(data);
    return crc;
}

int main(void) {
    printf("=== Test Suite: Winsock TCP & RSA/ISAAC Login Protocol ===\n");

    bool ws_ok = client_stream_global_init();
    assert(ws_ok);

    // Compute CRCs from real cache assets
    const char* pack_names[8] = {
        "title", "config", "interface", "media", "models", "textures", "wordenc", "sounds"
    };

    uint32_t crcs[9];
    crcs[0] = 0;
    for (int i = 0; i < 8; i++) {
        char path[256];
        snprintf(path, sizeof(path), "../engine/data/pack/client/%s", pack_names[i]);
        crcs[i + 1] = get_file_crc(path);
        if (crcs[i + 1] == 0) {
            snprintf(path, sizeof(path), "engine/data/pack/client/%s", pack_names[i]);
            crcs[i + 1] = get_file_crc(path);
        }
        printf("  CRC [%d] (%-10s): %u (0x%08X)\n", i + 1, pack_names[i], crcs[i + 1], crcs[i + 1]);
        assert(crcs[i + 1] != 0);
    }

    GameSession session;
    protocol_init_session(&session);
    protocol_set_crcs(&session, crcs);

    printf("[INFO] Connecting to 127.0.0.1:43594 and initiating login handshake...\n");
    const char *user=getenv("RS2_TEST_USERNAME"),*password=getenv("RS2_TEST_PASSWORD");
    if(!getenv("RS2_TEST_LOGIN") || !user || !password) {
        puts("[SKIP] Live login requires RS2_TEST_LOGIN and disposable test credentials; CRC checks passed only.");
        client_stream_global_cleanup();return 0;
    }
    int port=getenv("RS2_TEST_PORT") ? atoi(getenv("RS2_TEST_PORT")):43594;
    LoginResponse resp = protocol_login(&session,"127.0.0.1",port,user,password,false);

    if (resp == LOGIN_NET_ERROR) {
        printf("[WARN] Server not running on 127.0.0.1:43594 (skipping live packet test in offline environment)\n");
        client_stream_global_cleanup();
        printf("[SKIP] Live packet exchange: server unavailable; no live login pass.\n");
        return 0;
    }

    printf("[INFO] Server Login Response Code: %d\n", resp);
    if (resp == LOGIN_SUCCESS || resp == LOGIN_MOD_SUCCESS) {
        printf("[PASS] LOGIN SUCCESSFUL! Client session is now IN_GAME (is_mod=%d)\n", session.is_mod);
    } else {
        printf("[FAIL] Login failed with response code %d\n", resp);
        assert(resp == LOGIN_SUCCESS || resp == LOGIN_MOD_SUCCESS);
    }

    // Read incoming game packets
    uint8_t payload[8192];
    size_t pkt_len = 0;

    int packets_received = 0;
    while (packets_received < 3) {
        int opcode = protocol_read_packet(&session, payload, sizeof(payload), &pkt_len);
        if (opcode < 0) {
            printf("[WARN] Stream ended or error after %d packets\n", packets_received);
            break;
        }

        packets_received++;
        printf("[INFO] Received Game Packet #%d: Opcode=%d, Length=%zu bytes\n", packets_received, opcode, pkt_len);

        if (opcode == 237) { // REBUILD_NORMAL (Map Region)
            assert(pkt_len >= 4);
            int zoneX = (payload[0] << 8) | payload[1];
            int zoneZ = (payload[2] << 8) | payload[3];
            printf("  -> [REBUILD_NORMAL] Map Zone Center: (%d, %d)\n", zoneX, zoneZ);
        } else if (opcode == 140) { // LAST_LOGIN_INFO
            printf("  -> [LAST_LOGIN_INFO] Received last login metadata\n");
        } else if (opcode == 184) { // PLAYER_INFO
            printf("  -> [PLAYER_INFO] Initial player synchronization block received!\n");
        }
    }

    assert(packets_received > 0);
    printf("[PASS] Successfully received and decrypted %d live game packets via ISAAC!\n", packets_received);

    // Test sending client packets
    assert(protocol_send_idle(&session));
    printf("[PASS] Sent IDLE_TIMER (opcode 70)\n");

    int8_t rx[1] = {1};
    int8_t rz[1] = {0};
    assert(protocol_send_walk(&session, 3200, 3200, 1, rx, rz, false));
    printf("[PASS] Sent MOVE_GAMECLICK (opcode 181)\n");

    assert(protocol_send_button(&session, 2449));
    printf("[PASS] Sent IF_BUTTON (opcode 155)\n");

    protocol_close_session(&session);
    client_stream_global_cleanup();

    printf("=== ALL NETWORKING & LOGIN TESTS PASSED! ===\n");
    return 0;
}
