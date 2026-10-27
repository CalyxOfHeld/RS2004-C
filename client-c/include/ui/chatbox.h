#ifndef RS2_CHATBOX_H
#define RS2_CHATBOX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "core/wordfilter.h"
#include "entity/player_entity.h"
#include "net/protocol.h"
#include "gfx/pixfont.h"
#include "gfx/pix8.h"
#include "ui/context_menu.h"

#define CHATBOX_MAX_MESSAGES 100
#define CHATBOX_MAX_INPUT_LEN 80

typedef struct Chatbox {
    char *text[CHATBOX_MAX_MESSAGES];
    char *sender[CHATBOX_MAX_MESSAGES];
    int   type[CHATBOX_MAX_MESSAGES];
    int   message_count;

    char  input[CHATBOX_MAX_INPUT_LEN + 1];
    int   input_len;

    char  username[32];
    int   public_mode, private_mode, trade_mode, split_private;
    int   friend_count;
    uint64_t friend_ids[100];
    char friend_names[100][32];
    int friend_worlds[100];
    char friend_status[100][40];
    char ignore_names[100][32];
    int social_action;
    int report_interface;
    bool report_mute;
    char report_input[256],report_cursor[258];
    uint64_t ignore_ids[100];
    size_t ignore_count;
    int32_t private_message_ids[100];
    unsigned private_message_next;

    int   scroll_offset;
    bool  cursor_visible;
    int   cursor_cycle;
    bool private_prompt_open;
    uint64_t private_recipient;
    char private_input[81],private_prompt[80];
    int private_input_len;
    bool  count_dialog_open;
    char  count_dialog_input[16];
} Chatbox;

struct Component;
bool chatbox_report_key(Chatbox *chat,int viewport,int key);
void chatbox_prepare_report_component(Chatbox *chat,struct Component *component,bool rights,int cycle);
bool chatbox_open_social(Chatbox *chat,int action);
bool chatbox_social_action(Chatbox *chat,GameSession *session,int action,uint64_t name37,const char *self);
bool chatbox_submit_social(Chatbox *chat,GameSession *session,const char *self);
bool chatbox_open_private(Chatbox *chat,uint64_t recipient);
bool chatbox_send_private(Chatbox *chat,const WordFilterTables *tables,GameSession *session);
void chatbox_draw_private_prompt(Chatbox *chat,int x,int y,PixFont *font);
void chatbox_prepare_friend_component(Chatbox *chat,struct Component *component,int node_id);
bool chatbox_build_friend_menu(Chatbox *chat,struct Component *component,ContextMenu *menu);
void chatbox_init(Chatbox *chat, const char *username);
void chatbox_open_count_dialog(Chatbox *chat);
void chatbox_close_count_dialog(Chatbox *chat);
int chatbox_count_value(const Chatbox *chat);
void chatbox_add_message(Chatbox *chat, int type, const char *sender, const char *text);
bool chatbox_add_game_message225(Chatbox *chat,const uint8_t *payload,size_t length,bool override_chat,bool *added);
bool chatbox_add_game_message(Chatbox *chat, uint8_t *payload, size_t length);
/* Original packet21 replaces the ignore count/prefix; unused tail is retained.
 * Invalid lengths (>800 or not divisible by8) fail without mutation. */
bool chatbox_apply_ignores(Chatbox *chat,const uint8_t *payload,size_t length);
/* Original packet41. Suppressed messages return true without decoding/ring changes.
 * Admitted IDs enter the ring before decode, even if decode/filter fails (original
 * catch semantics). Short headers fail before mutation. No unfiltered fallback.
 * Packet payload is at most255 bytes; tables are the loaded revision225 cache. */
bool chatbox_add_private_message(Chatbox *chat,const WordFilterTables *tables,
    const uint8_t *payload,size_t length,bool override_chat);
/* Public0x40 admission updates actor text/colour/style/timer and history. Missing
 * player name represents original null; suppressed messages succeed unchanged.
 * Forced0x8 text is handled separately by the mask callback, as original. */
bool chatbox_add_public_message(Chatbox *chat,const WordFilterTables *tables,
    PlayerEntity *player,const uint8_t *packed,size_t length,int colour,int rights,bool override_chat);
/* Submit current nonempty ordinary input (codes32..122, at most80). Commands
 * are separate. Prefixes follow original sequential case-sensitive if statements.
 * Success queues wire bytes, publishes filtered local echo/actor state, clears
 * input and reveals public-mode2 as3. Failure preserves UI/actor/cipher/output
 * bytes, apart from stream out_overflow on capacity failure. Immediate socket
 * errors can have partial external transmission and require normal recovery. */
bool chatbox_send_public(Chatbox *chat,const WordFilterTables *tables,PlayerEntity *player,GameSession *session);
void chatbox_key_press(Chatbox *chat, char ch);
void chatbox_backspace(Chatbox *chat);
const char *chatbox_submit(Chatbox *chat);
const char *chatbox_take_input(Chatbox *chat);
void chatbox_draw(Chatbox *chat, Pix8 *chatback, int x, int y, PixFont *font_p12);
void chatbox_draw225(Chatbox *chat,Pix8 *chatback,int x,int y,PixFont *font,const char *login_name,Pix8 *scrollbar0,Pix8 *scrollbar1);
/* Revision225 client.isFriend includes the local player, case insensitive. */
bool chatbox_is_friend(const Chatbox *chat, const char *name);
bool chatbox_is_friend_as(const Chatbox *chat,const char *name,const char *self);
bool chatbox_apply_friend(Chatbox *chat, uint8_t *payload, size_t length, int node_id);
bool chatbox_message_visible(const Chatbox *chat, int index, bool split);
void chatbox_draw_private(Chatbox *chat, int x, int y, PixFont *font, int update_timer);
void chatbox_build_menu(Chatbox *chat, ContextMenu *menu, int mouse_x, int mouse_y,
                        bool split, bool rights, int update_timer);
void chatbox_free(Chatbox *chat);

#endif // RS2_CHATBOX_H
