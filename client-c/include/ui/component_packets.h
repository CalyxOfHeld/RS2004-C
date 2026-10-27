#ifndef RS2_COMPONENT_PACKETS_H
#define RS2_COMPONENT_PACKETS_H
#include "ui/component.h"
#include "ui/varp.h"
#include "net/protocol.h"
typedef struct { int viewport, sidebar, chat; } InterfaceState;
typedef struct {
    uint32_t last_address;
    int days_since_login,days_since_recovery,unread_messages;
    uint64_t dns_generation;
    char dns_request[16],dns[256],content[6][384];
} LastLoginInfo225;
struct Chatbox;
/* 1 stored,2 opened/requested DNS,-1 invalid,-2 output failure after fields/DNS.
 * Source closeInterfaces leaves report interface ID,count/social inputs intact. */
int last_login_receive225(LastLoginInfo225 *info,InterfaceState *interfaces,
    struct Chatbox *chat,ComponentConfig *components,GameSession *session,
    const uint8_t *data,size_t length);
void last_login_prepare225(LastLoginInfo225 *info,Component *component);
void interface_state_init(InterfaceState *state);
int interface_state_apply(InterfaceState *state, int opcode, const uint8_t *data, size_t length);
/* 1 applied, 0 not a component packet, -1 malformed. Uses production wire data. */
int component_apply_packet(ComponentConfig *cfg, VarpContext *varps, Arena *arena,
                           int opcode, const uint8_t *data, size_t length);
bool component_model_colourless225(const ComponentConfig *cfg,const Model *model);
void component_models_clear225(ComponentConfig *cfg);
void component_models_stats225(const ComponentConfig *cfg,int *owned,int *cached);
#endif
