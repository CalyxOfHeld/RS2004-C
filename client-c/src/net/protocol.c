#include "net/protocol.h"
#include "net/rsa.h"
#include "dash3d/pathfinder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int server_packet_sizes[256];
static bool packet_sizes_init = false;

static void init_packet_sizes(void) {
    if (packet_sizes_init) return;
    for (int i = 0; i < 256; i++) {
        server_packet_sizes[i] = -3; // unknown opcode must never be treated as empty
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

    // ServerGameZoneProt.ts: these may also arrive as standalone packets.
    server_packet_sizes[23]  = 14; // LOC_MERGE
    server_packet_sizes[42]  = 4;  // LOC_ANIM
    server_packet_sizes[49]  = 3;  // OBJ_DEL
    server_packet_sizes[50]  = 7;  // OBJ_REVEAL
    server_packet_sizes[59]  = 4;  // LOC_ADD_CHANGE
    server_packet_sizes[69]  = 15; // MAP_PROJANIM
    server_packet_sizes[76]  = 2;  // LOC_DEL
    server_packet_sizes[151] = 7;  // OBJ_COUNT
    server_packet_sizes[191] = 6;  // MAP_ANIM
    server_packet_sizes[223] = 5;  // OBJ_ADD

    packet_sizes_init = true;
}

void protocol_init_session(GameSession *session) {
    memset(session, 0, sizeof(GameSession));
    session->stream.sock = INVALID_SOCKET_VAL;
    session->packet_opcode = -1;
    init_packet_sizes();
}

void protocol_set_crcs(GameSession *session, const uint32_t crcs[9]) {
    memcpy(session->archive_crcs, crcs, sizeof(session->archive_crcs));
}

LoginResponse protocol_login(GameSession *session, const char *host, int port, const char *username, const char *password, bool low_mem) {
    init_packet_sizes();

    if (!client_stream_connect(&session->stream, host, port)) {
        fprintf(stderr, "[Login] Could not connect to %s:%d.\n", host, port);
        return LOGIN_NET_ERROR;
    }

    // 1. Read 8-byte server seed
    uint8_t seed_bytes[8];
    if (!client_stream_read(&session->stream, seed_bytes, 8)) {
        fprintf(stderr, "[Login] %s:%d did not provide the revision-225 server seed.\n", host, port);
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
        fprintf(stderr, "[Login] Failed to send login to %s:%d.\n", host, port);
        client_stream_close(&session->stream);
        return LOGIN_NET_ERROR;
    }

    // 8. Read 1-byte response code
    int resp = client_stream_read_byte(&session->stream);
    if (resp < 0) fprintf(stderr, "[Login] %s:%d closed or timed out before a login response.\n", host, port);
    if (resp == 2 || resp == 18) {
        session->in_game = true;
        session->is_mod = (resp == 18);
        return (LoginResponse)resp;
    }

    client_stream_close(&session->stream);
    return (LoginResponse)resp;
}

static int protocol_receive_error(GameSession *session, const char *reason) {
    fprintf(stderr, "[Protocol] Receive failed: %s\n", reason);
    protocol_close_session(session);
    return -1;
}

int protocol_poll_packet(GameSession *session, int *out_opcode, uint8_t *out_payload,
                         size_t capacity, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!session || !session->in_game || !out_opcode || !out_len || (!out_payload && capacity)) return -1;
    if (session->packet_opcode < 0) {
        uint8_t raw;
        int count = client_stream_read_some(&session->stream, &raw, 1);
        if (count < 0) return protocol_receive_error(session, "connection closed or socket error");
        if (!count) return 0;
        session->packet_opcode = (raw - isaac_next(&session->in_cipher)) & 255;
        session->packet_size = server_packet_sizes[session->packet_opcode];
        session->packet_header_read = session->packet_payload_read = 0;
        if (session->packet_size == -3) return protocol_receive_error(session, "unknown server opcode");
    }
    if (session->packet_size < 0) {
        size_t required = (size_t)-session->packet_size;
        int count = client_stream_read_some(&session->stream,
            session->packet_header + session->packet_header_read, required - session->packet_header_read);
        if (count < 0) return protocol_receive_error(session, "connection closed in packet length");
        session->packet_header_read += (size_t)count;
        if (session->packet_header_read < required) return 0;
        session->packet_size = required == 1 ? session->packet_header[0] :
            (session->packet_header[0] << 8) | session->packet_header[1];
    }
    size_t length = (size_t)session->packet_size;
    if (length > capacity || length > sizeof(session->stream.in_buf))
        return protocol_receive_error(session, "payload exceeds destination capacity");
    if (session->packet_payload_read < length) {
        int count = client_stream_read_some(&session->stream,
            session->stream.in_buf + session->packet_payload_read, length - session->packet_payload_read);
        if (count < 0) return protocol_receive_error(session, "connection closed in packet payload");
        session->packet_payload_read += (size_t)count;
        if (session->packet_payload_read < length) return 0;
    }
    if (length) memcpy(out_payload, session->stream.in_buf, length);
    *out_opcode = session->packet_opcode;
    *out_len = length;
    session->packet_opcode = -1;
    return 1;
}

int protocol_read_packet(GameSession *session, uint8_t *out_payload, size_t capacity, size_t *out_len) {
    int opcode;
    for (;;) {
        int result = protocol_poll_packet(session, &opcode, out_payload, capacity, out_len);
        if (result < 0) return -1;
        if (result > 0) return opcode;
        if (client_stream_wait_readable(&session->stream, 3000) <= 0)
            return protocol_receive_error(session, "timed out waiting for a complete packet");
    }
}

bool protocol_send_keepalive(GameSession *session) {
    if (!session || !session->in_game) return false;
    uint8_t opcode = (uint8_t)(108 + isaac_next(&session->out_cipher)); // NO_TIMEOUT
    return client_stream_write(&session->stream, &opcode, 1);
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

bool protocol_send_resume_p_countdialog(GameSession *session, int count) {
    if (!session || !session->in_game) return false;
    uint8_t buf[5];
    buf[0] = (uint8_t)((237 + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((count >> 24) & 0xFF);
    buf[2] = (uint8_t)((count >> 16) & 0xFF);
    buf[3] = (uint8_t)((count >> 8) & 0xFF);
    buf[4] = (uint8_t)(count & 0xFF);
    return client_stream_write(&session->stream, buf, 5);
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

bool protocol_send_design(GameSession *session,int gender,const int parts[7],const int colors[5]) {
    if(!session || !session->in_game || !parts || !colors || gender<0 || gender>1) return false;
    uint8_t bytes[14];Packet p=packet_wrap(bytes,sizeof(bytes));
    p1(&p,52+isaac_next(&session->out_cipher));p1(&p,gender);
    for(int i=0;i<7;i++) p1(&p,parts[i]);
    for(int i=0;i<5;i++) p1(&p,colors[i]);
    return client_stream_write(&session->stream,bytes,p.pos);
}

bool protocol_send_item_on_item(GameSession *session,int item,int slot,int component,int use_item,int use_slot,int use_component) {
    if(!session || !session->in_game) return false;
    uint8_t bytes[13];Packet p=packet_wrap(bytes,sizeof(bytes));
    p1(&p,130+isaac_next(&session->out_cipher));
    p2(&p,item);p2(&p,slot);p2(&p,component);
    p2(&p,use_item);p2(&p,use_slot);p2(&p,use_component);
    return client_stream_write(&session->stream,bytes,p.pos);
}

static bool send_walk_opcode(GameSession *session,int opcode,int start_x,int start_z,int count,const int8_t *rel_x,const int8_t *rel_z,bool ctrl,const uint8_t *minimap_extra);
static bool send_route(GameSession *session,int opcode,int base_x,int base_z,int source_x,int source_z,const int *x,const int *z,int count,bool ctrl,const uint8_t *minimap_extra) {
    if(!x || !z || count<=0 || count>MAX_PATH_STEPS) return false;
    // Client.tryMove backtracks direction changes, then sends the nearest
    // turn first. Including the origin separately retains a turn on step 1.
    int indices[25],points=0,previous_x=source_x,previous_z=source_z;
    for(int i=0;i<count;i++) {
        int dx=x[i]-previous_x,dz=z[i]-previous_z;
        if(i==count-1 || dx!=x[i+1]-x[i] || dz!=z[i+1]-z[i]) {
            indices[points++]=i;if(points==25) break; // reference max 25 turns
        }
        previous_x=x[i];previous_z=z[i];
    }
    if (!points) return false;
    int first=indices[0];int8_t rx[24],rz[24];
    for(int i=1;i<points;i++) {
        int dx=x[indices[i]]-x[first],dz=z[indices[i]]-z[first];
        if(dx < -128 || dx>127 || dz < -128 || dz>127) return false;
        rx[i-1]=(int8_t)dx;rz[i-1]=(int8_t)dz;
    }
    return send_walk_opcode(session,opcode,base_x+x[first],base_z+z[first],points-1,rx,rz,ctrl,minimap_extra);
}
bool protocol_send_route(GameSession *session,int bx,int bz,int sx,int sz,const int *x,const int *z,int count,bool ctrl) {return send_route(session,181,bx,bz,sx,sz,x,z,count,ctrl,NULL);}
bool protocol_send_operation_route(GameSession *session,int bx,int bz,int sx,int sz,const int *x,const int *z,int count,bool ctrl) {return send_route(session,93,bx,bz,sx,sz,x,z,count,ctrl,NULL);}

bool protocol_send_minimap_route(GameSession *session,int bx,int bz,int sx,int sz,
    const int *x,const int *z,int count,bool ctrl,int click_x,int click_y,int yaw,int player_x,int player_z,bool nearest) {
    if(click_x < -73 || click_x > 72 || click_y < -75 || click_y > 75 ||
        yaw < 0 || yaw > 2047 || player_x < 0 || player_x > 65535 || player_z < 0 || player_z > 65535) return false;
    // Original handleMinimapInput trailer; current minimap angle/zoom are zero.
    uint8_t extra[14];Packet p=packet_wrap(extra,sizeof(extra));
    p1(&p,click_x);p1(&p,click_y);p2(&p,yaw);p1(&p,57);
    p1(&p,0);p1(&p,0);p1(&p,89);p2(&p,player_x);p2(&p,player_z);
    p1(&p,nearest ? 1 : 0);p1(&p,63);
    return send_route(session,165,bx,bz,sx,sz,x,z,count,ctrl,extra);
}

bool protocol_send_opplayer(GameSession *session,int op,int slot) {
    static const int opcodes[4]={164,53,185,206};
    if(!session || !session->in_game || op<1 || op>4 || slot<0 || slot>=2047) return false;
    uint8_t data[3]={(uint8_t)(opcodes[op-1]+isaac_next(&session->out_cipher)),(uint8_t)(slot>>8),(uint8_t)slot};
    return client_stream_write(&session->stream,data,sizeof(data));
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

bool protocol_send_inv_buttond(GameSession *session, int component_id, int from_slot, int to_slot) {
    if (!session || !session->in_game) return false;
    uint8_t buf[7];
    buf[0] = (uint8_t)((159 + isaac_next(&session->out_cipher)) & 0xFF);
    buf[1] = (uint8_t)((component_id >> 8) & 0xFF);
    buf[2] = (uint8_t)(component_id & 0xFF);
    buf[3] = (uint8_t)((from_slot >> 8) & 0xFF);
    buf[4] = (uint8_t)(from_slot & 0xFF);
    buf[5] = (uint8_t)((to_slot >> 8) & 0xFF);
    buf[6] = (uint8_t)(to_slot & 0xFF);
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

bool protocol_send_walk(GameSession *session,int start_x,int start_z,int count,const int8_t *rel_x,const int8_t *rel_z,bool ctrl) {
    return send_walk_opcode(session,181,start_x,start_z,count,rel_x,rel_z,ctrl,NULL);
}
static bool send_walk_opcode(GameSession *session,int opcode,int start_x,int start_z,int count,const int8_t *rel_x,const int8_t *rel_z,bool ctrl,const uint8_t *minimap_extra) {
    if (!session || !session->in_game) return false;
    uint8_t buf[256];
    if (count < 0 || count > 24 || (count && (!rel_x || !rel_z))) return false;
    size_t payload_len = 1 + 2 + 2 + (size_t)count * 2 + (minimap_extra ? 14 : 0);
    if (payload_len + 2 > sizeof(buf)) return false;

    Packet pkt = packet_wrap(buf, sizeof(buf));
    p1(&pkt, (opcode + isaac_next(&session->out_cipher)) & 0xFF);
    p1(&pkt, (int32_t)payload_len);
    p1(&pkt, ctrl ? 1 : 0);
    p2(&pkt, start_x);
    p2(&pkt, start_z);
    for (int i = 0; i < count; i++) {
        p1(&pkt, rel_x[i]);
        p1(&pkt, rel_z[i]);
    }
    if (minimap_extra) pdata(&pkt,minimap_extra,14);
    return client_stream_write(&session->stream, buf, pkt.pos);
}

bool protocol_send_command(GameSession *session, const char *cmd) {
    if (!session || !session->in_game || !cmd) return false;
    size_t len = strlen(cmd);
    uint8_t buf[512];
    if (len + 3 > sizeof(buf) || len + 1 > 255) return false;

    Packet pkt = packet_wrap(buf, sizeof(buf));
    p1(&pkt, (4 + isaac_next(&session->out_cipher)) & 0xFF);
    p1(&pkt, (int32_t)len + 1);
    for (size_t i = 0; i < len; i++) {
        p1(&pkt, (uint8_t)cmd[i]);
    }
    p1(&pkt, 10); // ClientCheatDecoder.gjstr expects the revision-225 delimiter.
    return client_stream_write(&session->stream, buf, pkt.pos);
}

void protocol_close_session(GameSession *session) {
    if (!session) return;
    client_stream_close(&session->stream);
    session->in_game = false;
    session->packet_opcode = -1;
    session->packet_header_read = session->packet_payload_read = 0;
}
