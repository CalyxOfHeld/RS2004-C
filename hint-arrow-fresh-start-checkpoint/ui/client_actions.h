#ifndef RS2_CLIENT_ACTIONS_H
#define RS2_CLIENT_ACTIONS_H
#include "ui/context_menu.h"
#include "ui/component.h"
#include "ui/varp.h"
#include "ui/chatbox.h"
#include "config/obj.h"
#include "net/protocol.h"
#include "config/idk.h"
#include "entity/player_entity.h"
#include "net/entity_sync.h"
#include "dash3d/pathfinder.h"
#include "dash3d/map_streamer.h"
typedef struct {int gender,parts[7],colors[5];bool initialized;} ClientDesign;
typedef struct {
    GameSession *session;
    ComponentConfig *components;
    VarpContext *varps;
    ObjConfig *objects;
    MenuSelection *selection;
    Chatbox *chatbox;
    int *viewport_interface, *chat_interface;
    bool live,trace;
    IdkConfig *idkits;
    ClientDesign *design;
    EntitySyncContext *entities;
    World3D *scene;
    LocConfig *locations;
    Pathfinder *pathfinder;
    CollisionMap *collision;
    MapStreamer *map;
} ClientActions;
typedef enum { ACTION_UNAVAILABLE, ACTION_LOCAL, ACTION_SENT, ACTION_WALK, ACTION_FAILED } ActionResult;
void client_design_init(ClientDesign *design,IdkConfig *kits);
void client_design_prepare(ClientActions *ctx,Component *root,Arena *frame,int cycle);
ActionResult client_action_dispatch(ClientActions *ctx,MenuAction action);
#endif
