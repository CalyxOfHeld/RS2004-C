#ifndef RS2_PROTOCOL_H
#define RS2_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "core/packet.h"
#include "core/isaac.h"
#include "net/client_stream.h"

typedef enum {
    LOGIN_RETRY = 1,
    LOGIN_SUCCESS = 2,
    LOGIN_INVALID_CREDENTIALS = 3,
    LOGIN_ACCOUNT_LOGGED_IN = 5,
    LOGIN_REVISION_CRC_ERROR = 6,
    LOGIN_WORLD_FULL = 7,
    LOGIN_RECONNECT_SUCCESS = 15,
    LOGIN_RATE_LIMITED = 16,
    LOGIN_MOD_SUCCESS = 18,
    LOGIN_NET_ERROR = -1
} LoginResponse;

typedef struct {
    ClientStream stream;
    Isaac in_cipher;
    Isaac out_cipher;
    bool in_game;
    bool is_mod;
    bool protocol_error; /* last receive failure was a framing error, not I/O (client.java T1/T2) */
    uint32_t archive_crcs[9];
    int packet_opcode; /* -1 until the next encrypted opcode is available */
    int packet_size;
    uint8_t packet_header[2];
    size_t packet_header_read;
    size_t packet_payload_read;
} GameSession;

void protocol_init_session(GameSession *session);
void protocol_set_crcs(GameSession *session, const uint32_t crcs[9]);
LoginResponse protocol_login(GameSession *session, const char *host, int port, const char *username, const char *password, bool low_mem);
/* client.login(): opcode 18 when reconnecting, otherwise 16. Reply 1 is retried
   after two seconds; replies 2, 18 and 15 leave the session in game. */
LoginResponse protocol_login_ex(GameSession *session, const char *host, int port, const char *username, const char *password, bool low_mem, bool reconnect);
int protocol_read_packet(GameSession *session, uint8_t *out_payload, size_t capacity, size_t *out_len);
int protocol_poll_packet(GameSession *session, int *out_opcode, uint8_t *out_payload, size_t capacity, size_t *out_len);

bool protocol_send_idle(GameSession *session);
bool protocol_send_keepalive(GameSession *session);
/* The ANTICHEAT_CYCLELOGIC packets (ClientGameProt.ts). 2 and 6 draw Math.random. */
bool protocol_send_cyclelogic1(GameSession *session);
bool protocol_send_cyclelogic2(GameSession *session);
bool protocol_send_cyclelogic3(GameSession *session);
bool protocol_send_cyclelogic4(GameSession *session);
bool protocol_send_cyclelogic5(GameSession *session);
bool protocol_send_cyclelogic6(GameSession *session);
/* EVENT_CAMERA_POSITION (189): client.updateGame while an arrow key is held. */
bool protocol_send_camera_position(GameSession *session, int pitch, int yaw, int minimap_angle, int minimap_zoom);
/* REBUILD_NORMAL's map request (opcode 150, var-byte): (type, x, z) per missing file;
   sent even when nothing is missing. */
bool protocol_send_map_request(GameSession *session, const uint8_t *entries, int length);
bool protocol_send_button(GameSession *session, int button_id);
bool protocol_send_close_modal(GameSession *session);
bool protocol_send_resume_pausebutton(GameSession *session, int component_id);
bool protocol_send_resume_p_countdialog(GameSession *session, int count);
bool protocol_send_chat_setmode(GameSession *session, int public_mode, int private_mode, int trade_mode);
bool protocol_send_opheld(GameSession *session, int op, int item_id, int slot, int component_id);
bool protocol_send_design(GameSession *session,int gender,const int parts[7],const int colors[5]);
bool protocol_send_item_on_item(GameSession *session,int item,int slot,int component,int use_item,int use_slot,int use_component);
bool protocol_send_route(GameSession *session,int base_x,int base_z,int source_x,int source_z,const int *x,const int *z,int count,bool ctrl);
bool protocol_send_operation_route(GameSession *session,int base_x,int base_z,int source_x,int source_z,const int *x,const int *z,int count,bool ctrl);
bool protocol_send_minimap_route(GameSession *session,int base_x,int base_z,int source_x,int source_z,
    const int *x,const int *z,int count,bool ctrl,int click_x,int click_y,int yaw,int minimap_angle,int minimap_zoom,
    int player_x,int player_z,bool nearest);
bool protocol_send_opplayer(GameSession *session,int op,int slot);
bool protocol_send_inv_button(GameSession *session, int op, int item_id, int slot, int component_id);
bool protocol_send_inv_buttond(GameSession *session, int component_id, int from_slot, int to_slot);
bool protocol_send_opnpc(GameSession *session, int op, int npc_slot);
bool protocol_send_oploc(GameSession *session, int op, int x, int z, int loc_id);
bool protocol_send_opobj(GameSession *session, int op, int x, int z, int obj_id);
bool protocol_send_walk(GameSession *session, int start_x, int start_z, int count, const int8_t *rel_x, const int8_t *rel_z, bool ctrl);
bool protocol_send_command(GameSession *session, const char *cmd);

void protocol_close_session(GameSession *session);

typedef enum { WORLD_NPC, WORLD_LOC, WORLD_OBJ, WORLD_PLAYER } WorldTargetKind;
bool protocol_send_target_item(GameSession *session, WorldTargetKind kind, int target, int x, int z,
    int use_item, int use_slot, int use_component);
bool protocol_send_target_spell(GameSession *session, WorldTargetKind kind, int target, int x, int z, int spell_component);
bool protocol_send_spell_on_item(GameSession *session, int item, int slot, int component, int spell_component);

#endif // RS2_PROTOCOL_H
