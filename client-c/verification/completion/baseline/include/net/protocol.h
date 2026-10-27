#ifndef RS2_PROTOCOL_H
#define RS2_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "core/packet.h"
#include "core/isaac.h"
#include "net/client_stream.h"

typedef enum {
    LOGIN_SUCCESS = 2,
    LOGIN_INVALID_CREDENTIALS = 3,
    LOGIN_ACCOUNT_LOGGED_IN = 5,
    LOGIN_REVISION_CRC_ERROR = 6,
    LOGIN_WORLD_FULL = 7,
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
    uint32_t archive_crcs[9];
} GameSession;

void protocol_init_session(GameSession *session);
void protocol_set_crcs(GameSession *session, const uint32_t crcs[9]);
LoginResponse protocol_login(GameSession *session, const char *host, int port, const char *username, const char *password, bool low_mem);
int protocol_read_packet(GameSession *session, uint8_t *out_payload, size_t *out_len);
int protocol_poll_packet(GameSession *session, int *out_opcode, uint8_t *out_payload, size_t *out_len);

bool protocol_send_idle(GameSession *session);
bool protocol_send_button(GameSession *session, int button_id);
bool protocol_send_close_modal(GameSession *session);
bool protocol_send_resume_pausebutton(GameSession *session, int component_id);
bool protocol_send_chat_setmode(GameSession *session, int public_mode, int private_mode, int trade_mode);
bool protocol_send_opheld(GameSession *session, int op, int item_id, int slot, int component_id);
bool protocol_send_inv_button(GameSession *session, int op, int item_id, int slot, int component_id);
bool protocol_send_opnpc(GameSession *session, int op, int npc_slot);
bool protocol_send_oploc(GameSession *session, int op, int x, int z, int loc_id);
bool protocol_send_opobj(GameSession *session, int op, int x, int z, int obj_id);
bool protocol_send_walk(GameSession *session, int start_x, int start_z, int count, const int8_t *rel_x, const int8_t *rel_z, bool ctrl);
bool protocol_send_command(GameSession *session, const char *cmd);

void protocol_close_session(GameSession *session);

#endif // RS2_PROTOCOL_H
