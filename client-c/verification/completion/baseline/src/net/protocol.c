#include "net/protocol.h"
#include "net/rsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int server_packet_sizes[256];
static bool packet_sizes_init = false;

static void init_packet_sizes(void) {
    if (packet_sizes_init) return;
    for (int i = 0; i < 256; i++) {
        server_packet_sizes[i] = 0;
    }

    // ServerGameProt Rev 225 mappings
    server_packet_sizes[1]   = -2; // NPC_INFO
    server_packet_sizes[2]   = 4;  // IF_SETCOLOUR
    server_packet_sizes[3]   = 6;  // CAM_MOVETO
    server_packet_sizes[4]   = -1; // MESSAGE_GAME
    server_packet_sizes[7]   = 2;  // UPDATE_ZONE_PARTIAL_FOLLOWS
    server_packet_sizes[12]  = 5;  // SYNTH_SOUND
    server_packet_sizes[13]  = 4;  // CAM_SHAKE
    server_packet_sizes[14]  = 2;  // IF_OPENCHAT
    server_packet_sizes[15]  = 2;  // UPDATE_INV_STOP_TRANSMIT
    server_packet_sizes[19]  = 0;  // UNSET_MAP_FLAG
    server_packet_sizes[20]  = 2;  // DATA_LOC_DONE
    server_packet_sizes[21]  = -2; // UPDATE_IGNORELIST
    server_packet_sizes[22]  = 2;  // UPDATE_RUNWEIGHT
    server_packet_sizes[25]  = 6;  // HINT_ARROW
    server_packet_sizes[26]  = 3;  // IF_SETHIDE
    server_packet_sizes[28]  = 4;  // IF_OPENMAIN_SIDE
    server_packet_sizes[32]  = 3;  // CHAT_FILTER_SETTINGS
    server_packet_sizes[41]  = -1; // MESSAGE_PRIVATE
    server_packet_sizes[43]  = 2;  // UPDATE_REBOOT_TIMER
    server_packet_sizes[44]  = 6;  // UPDATE_STAT
    server_packet_sizes[46]  = 6;  // IF_SETOBJECT
    server_packet_sizes[54]  = -1; // MIDI_SONG
    server_packet_sizes[68]  = 1;  // UPDATE_RUNENERGY
    server_packet_sizes[74]  = 6;  // CAM_LOOKAT
    server_packet_sizes[80]  = 2;  // DATA_LAND_DONE
    server_packet_sizes[84]  = 1;  // IF_SETTAB_ACTIVE
    server_packet_sizes[87]  = 4;  // IF_SETMODEL
    server_packet_sizes[98]  = -2; // UPDATE_INV_FULL
    server_packet_sizes[103] = 6;  // IF_SETRECOL
    server_packet_sizes[126] = 1;  // TUT_FLASH
    server_packet_sizes[129] = 0;  // IF_CLOSE
    server_packet_sizes[132] = -2; // DATA_LAND
    server_packet_sizes[133] = 0;  // FINISH_TRACKING
    server_packet_sizes[135] = 2;  // UPDATE_ZONE_FULL_FOLLOWS
    server_packet_sizes[136] = 0;  // RESET_ANIMS
    server_packet_sizes[139] = 2;  // UPDATE_PID
    server_packet_sizes[140] = 9;  // LAST_LOGIN_INFO
    server_packet_sizes[142] = 0;  // LOGOUT
    server_packet_sizes[146] = 4;  // IF_SETANIM
    server_packet_sizes[150] = 3;  // VARP_SMALL
    server_packet_sizes[152] = 9;  // UPDATE_FRIENDLIST
    server_packet_sizes[162] = -2; // UPDATE_ZONE_PARTIAL_ENCLOSED
    server_packet_sizes[167] = 3;  // IF_SETTAB
    server_packet_sizes[168] = 2;  // IF_OPENMAIN
    server_packet_sizes[175] = 6;  // VARP_LARGE
    server_packet_sizes[184] = -2; // PLAYER_INFO
    server_packet_sizes[185] = 2;  // TUT_OPEN
    server_packet_sizes[193] = 0;  // RESET_CLIENT_VARCACHE
    server_packet_sizes[195] = 2;  // IF_OPENSIDE
    server_packet_sizes[197] = 2;  // IF_SETPLAYERHEAD
    server_packet_sizes[201] = -2; // IF_SETTEXT
    server_packet_sizes[204] = 4;  // IF_SETNPCHEAD
    server_packet_sizes[209] = 6;  // IF_SETPOSITION
    server_packet_sizes[212] = -2; // MIDI_JINGLE
    server_packet_sizes[213] = -2; // UPDATE_INV_PARTIAL
    server_packet_sizes[220] = -2; // DATA_LOC
    server_packet_sizes[226] = 0;  // ENABLE_TRACKING
    server_packet_sizes[237] = -2; // REBUILD_NORMAL
    server_packet_sizes[239] = 0;  // CAM_RESET
    server_packet_sizes[243] = 0;  // P_COUNTDIALOG
    server_packet_sizes[254] = 1;  // SET_MULTIWAY

    packet_sizes_init = true;
}

void protocol_init_session(GameSession *session) {
    memset(session, 0, sizeof(GameSession));
    init_packet_sizes();
}

void protocol_set_crcs(GameSession *session, const uint32_t crcs[9]) {
    memcpy(session->archive_crcs, crcs, sizeof(session->archive_crcs));
}

LoginResponse protocol_login(GameSession *session, const char *host, int port, const char *username, const char *password, bool low_mem) {
    init_packet_sizes();

    if (!client_stream_connect(&session->stream, host, port)) {
        return LOGIN_NET_ERROR;
    }

    // 1. Read 8-byte server seed
    uint8_t seed_bytes[8];
    if (!client_stream_read(&session->stream, seed_bytes, 8)) {
        client_stream_close(&session->stream);
        return LOGIN_NET_ERROR;
    }

    uint64_t server_seed = 0;
    for (int i = 0; i < 8; i++) {
        server_seed = (server_seed << 8) | seed_bytes[i];
    }

    // 2. Generate 4-part ISAAC seed
    uint32_t seed[4];
    seed[0] = (uint32_t)rand();
    seed[1] = (uint32_t)rand();
    seed[2] = (uint32_t)(server_seed >> 32);
    seed[3] = (uint32_t)(server_seed & 0xFFFFFFFF);

    // 3. Prepare RSA block
    uint8_t rsa_buf[512];
    Packet rsa_pkt = packet_wrap(rsa_buf, sizeof(rsa_buf));
    p1(&rsa_pkt, 10);
    p4(&rsa_pkt, (int32_t)seed[0]);
    p4(&rsa_pkt, (int32_t)seed[1]);
    p4(&rsa_pkt, (int32_t)seed[2]);
    p4(&rsa_pkt, (int32_t)seed[3]);
    p4(&rsa_pkt, 1); // uid
    pjstr(&rsa_pkt, username);
    pjstr(&rsa_pkt, password);

    // 4. Encrypt RSA block
    uint8_t cipher_buf[256];
    size_t cipher_len = rsa_encrypt(rsa_buf, rsa_pkt.pos, cipher_buf, sizeof(cipher_buf));

    // 5. Build Login packet (Opcode 16)
    uint8_t login_buf[1024];
    Packet login_pkt = packet_wrap(login_buf, sizeof(login_buf));
    p1(&login_pkt, 16); // opcode 16
    size_t payload_len = 1 + 1 + 36 + 1 + cipher_len;
    p1(&login_pkt, (int32_t)payload_len);
    p1(&login_pkt, 225); // revision 225
    p1(&login_pkt, low_mem ? 1 : 0);
    for (int i = 0; i < 9; i++) {
        p4(&login_pkt, (int32_t)session->archive_crcs[i]);
    }
    p1(&login_pkt, (int32_t)cipher_len);
    pdata(&login_pkt, cipher_buf, cipher_len);

    // 6. Initialize ISAAC ciphers
    isaac_init(&session->out_cipher, seed, 4);
    uint32_t in_seed[4];
    for (int i = 0; i < 4; i++) {
        in_seed[i] = seed[i] + 50;
    }
    isaac_init(&session->in_cipher, in_seed, 4);

    // 7. Send login packet
    if (!client_stream_write(&session->stream, login_buf, login_pkt.pos)) {
        client_stream_close(&session->stream);
        return LOGIN_NET_ERROR;
    }

    // 8. Read 1-byte response code
    int resp = client_stream_read_byte(&session->stream);
    if (resp == 2 || resp == 18) {
        session->in_game = true;
        session->is_mod = (resp == 18);
        return (LoginResponse)resp;
    }

    client_stream_close(&session->stream);
    return (LoginResponse)resp;
}

int protocol_read_packet(GameSession *session, uint8_t *out_payload, size_t *out_len) {
    if (!session || !session->in_game) return -1;

    int raw_op = client_stream_read_byte(&session->stream);
    if (raw_op < 0) return -1;

    int opcode = (raw_op - isaac_next(&session->in_cipher)) & 0xFF;

    int sz = server_packet_sizes[opcode];
    size_t len = 0;

    if (sz == -1) {
        int b = client_stream_read_byte(&session->stream);
        if (b < 0) return -1;
        len = (size_t)b;
    } else if (sz == -2) {
        int b1 = client_stream_read_byte(&session->stream);
        int b2 = client_stream_read_byte(&session->stream);
        if (b1 < 0 || b2 < 0) return -1;
        len = (size_t)((b1 << 8) | b2);
    } else {
        len = (size_t)sz;
    }

    if (len > 0) {
        if (!client_stream_read(&session->stream, out_payload, len)) {
            return -1;
        }
    }

    *out_len = len;
    return opcode;
}

int protocol_poll_packet(GameSession *session, int *out_opcode, uint8_t *out_payload, size_t *out_len) {
    if (!session || !session->in_game) return -1;
    if (client_stream_available(&session->stream) <= 0) {
        return 0; // No data pending
    }

    int op = protocol_read_packet(session, out_payload, out_len);
    if (op < 0) return -1;

    *out_opcode = op;
    return 1; // Packet received
}

bool protocol_send_idle(GameSession *session) {
    if (!session || !session->in_game) return false;
    uint8_t op = (uint8_t)((70 + isaac_next(&session->out_cipher)) & 0xFF);
    return client_stream_write(&session->stream, &op, 1);
}

bool protocol_send_button(GameSession *session, int button_id) {
    if (!session || !session->in_game) return false;
    uint8_t buf[3];
    buf[0] = (uint8_t)((155 + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((button_id >> 8) & 0xFF);
    buf[2] = (uint8_t)(button_id & 0xFF);
    return client_stream_write(&session->stream, buf, 3);
}

bool protocol_send_close_modal(GameSession *session) {
    if (!session || !session->in_game) return false;
    uint8_t op = (uint8_t)((231 + isaac_next(&session->out_cipher)) & 0xFF);
    return client_stream_write(&session->stream, &op, 1);
}

bool protocol_send_resume_pausebutton(GameSession *session, int component_id) {
    if (!session || !session->in_game) return false;
    uint8_t buf[3];
    buf[0] = (uint8_t)((235 + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((component_id >> 8) & 0xFF);
    buf[2] = (uint8_t)(component_id & 0xFF);
    return client_stream_write(&session->stream, buf, 3);
}

bool protocol_send_chat_setmode(GameSession *session, int public_mode, int private_mode, int trade_mode) {
    if (!session || !session->in_game) return false;
    uint8_t buf[4];
    buf[0] = (uint8_t)((244 + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)(public_mode & 0xFF);
    buf[2] = (uint8_t)(private_mode & 0xFF);
    buf[3] = (uint8_t)(trade_mode & 0xFF);
    return client_stream_write(&session->stream, buf, 4);
}

bool protocol_send_opheld(GameSession *session, int op, int item_id, int slot, int component_id) {
    if (!session || !session->in_game) return false;
    static const int opheld_opcodes[5] = { 195, 71, 133, 157, 211 };
    if (op < 1 || op > 5) return false;
    uint8_t buf[7];
    buf[0] = (uint8_t)((opheld_opcodes[op - 1] + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((item_id >> 8) & 0xFF);
    buf[2] = (uint8_t)(item_id & 0xFF);
    buf[3] = (uint8_t)((slot >> 8) & 0xFF);
    buf[4] = (uint8_t)(slot & 0xFF);
    buf[5] = (uint8_t)((component_id >> 8) & 0xFF);
    buf[6] = (uint8_t)(component_id & 0xFF);
    return client_stream_write(&session->stream, buf, 7);
}

bool protocol_send_inv_button(GameSession *session, int op, int item_id, int slot, int component_id) {
    if (!session || !session->in_game) return false;
    static const int inv_button_opcodes[5] = { 31, 59, 212, 38, 6 };
    if (op < 1 || op > 5) return false;
    uint8_t buf[7];
    buf[0] = (uint8_t)((inv_button_opcodes[op - 1] + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((item_id >> 8) & 0xFF);
    buf[2] = (uint8_t)(item_id & 0xFF);
    buf[3] = (uint8_t)((slot >> 8) & 0xFF);
    buf[4] = (uint8_t)(slot & 0xFF);
    buf[5] = (uint8_t)((component_id >> 8) & 0xFF);
    buf[6] = (uint8_t)(component_id & 0xFF);
    return client_stream_write(&session->stream, buf, 7);
}

bool protocol_send_opnpc(GameSession *session, int op, int npc_slot) {
    if (!session || !session->in_game) return false;
    static const int opnpc_opcodes[5] = { 194, 8, 27, 113, 100 };
    if (op < 1 || op > 5) return false;
    uint8_t buf[3];
    buf[0] = (uint8_t)((opnpc_opcodes[op - 1] + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((npc_slot >> 8) & 0xFF);
    buf[2] = (uint8_t)(npc_slot & 0xFF);
    return client_stream_write(&session->stream, buf, 3);
}

bool protocol_send_oploc(GameSession *session, int op, int x, int z, int loc_id) {
    if (!session || !session->in_game) return false;
    static const int oploc_opcodes[5] = { 245, 172, 96, 97, 116 };
    if (op < 1 || op > 5) return false;
    uint8_t buf[7];
    buf[0] = (uint8_t)((oploc_opcodes[op - 1] + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((x >> 8) & 0xFF);
    buf[2] = (uint8_t)(x & 0xFF);
    buf[3] = (uint8_t)((z >> 8) & 0xFF);
    buf[4] = (uint8_t)(z & 0xFF);
    buf[5] = (uint8_t)((loc_id >> 8) & 0xFF);
    buf[6] = (uint8_t)(loc_id & 0xFF);
    return client_stream_write(&session->stream, buf, 7);
}

bool protocol_send_opobj(GameSession *session, int op, int x, int z, int obj_id) {
    if (!session || !session->in_game) return false;
    static const int opobj_opcodes[5] = { 140, 40, 200, 178, 247 };
    if (op < 1 || op > 5) return false;
    uint8_t buf[7];
    buf[0] = (uint8_t)((opobj_opcodes[op - 1] + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((x >> 8) & 0xFF);
    buf[2] = (uint8_t)(x & 0xFF);
    buf[3] = (uint8_t)((z >> 8) & 0xFF);
    buf[4] = (uint8_t)(z & 0xFF);
    buf[5] = (uint8_t)((obj_id >> 8) & 0xFF);
    buf[6] = (uint8_t)(obj_id & 0xFF);
    return client_stream_write(&session->stream, buf, 7);
}

bool protocol_send_walk(GameSession *session, int start_x, int start_z, int count, const int8_t *rel_x, const int8_t *rel_z, bool ctrl) {
    if (!session || !session->in_game) return false;
    uint8_t buf[256];
    size_t payload_len = 1 + 2 + 2 + count * 2;
    if (payload_len + 2 > sizeof(buf)) return false;

    Packet pkt = packet_wrap(buf, sizeof(buf));
    p1(&pkt, (181 + isaac_next(&session->out_cipher)) & 0xFF);
    p1(&pkt, (int32_t)payload_len);
    p1(&pkt, ctrl ? 1 : 0);
    p2(&pkt, start_x);
    p2(&pkt, start_z);
    for (int i = 0; i < count; i++) {
        p1(&pkt, rel_x[i]);
        p1(&pkt, rel_z[i]);
    }
    return client_stream_write(&session->stream, buf, pkt.pos);
}

bool protocol_send_command(GameSession *session, const char *cmd) {
    if (!session || !session->in_game || !cmd) return false;
    size_t len = strlen(cmd);
    uint8_t buf[512];
    if (len + 3 > sizeof(buf)) return false;

    Packet pkt = packet_wrap(buf, sizeof(buf));
    p1(&pkt, (4 + isaac_next(&session->out_cipher)) & 0xFF);
    p1(&pkt, (int32_t)len);
    for (size_t i = 0; i < len; i++) {
        p1(&pkt, (uint8_t)cmd[i]);
    }
    return client_stream_write(&session->stream, buf, pkt.pos);
}

void protocol_close_session(GameSession *session) {
    if (!session) return;
    client_stream_close(&session->stream);
    session->in_game = false;
}
