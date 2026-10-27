#ifndef RS2_OVERHEAD_CHAT_H
#define RS2_OVERHEAD_CHAT_H
#include "ui/chatbox.h"
#include "net/entity_sync.h"
#define OVERHEAD_CHAT_LIMIT 50
typedef struct {
    int count;
    int x[50],y[50],width[50],height[50],colour[50],style[50],timer[50];
    bool present[50];char text[50][256];
} OverheadChat;
/* Persistent list matches original arrays, including unused next-slot writes.
 * Project returns original viewport-relative coordinates (only x>=0 admitted).
 * Local/active players then NPCs; actual visibility and friend/public rules.
 * Native empty chat with positive timer represents original non-null empty text.
 * Draw at origin in a512x334 viewport; scroll resets clip to that viewport, as
 * original resetClipping on areaViewport. Other styles preserve current clip.
 * Safe false for malformed metadata or original styled50th-entry next-slot OOB;
 * collection state retained but no chat pixels drawn on that failure. */
bool overhead_chat_draw(OverheadChat *list,const EntitySyncContext *entities,const Chatbox *chat,PixFont *font,
    bool (*project)(void*,const PathingEntity*,int*,int*),void *user,int cycle,int effects,int origin_x,int origin_y);
#endif
