#include "ui/client_actions.h"
#include "audio/midi.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gfx/pix3d.h"

static void design_validate(ClientDesign *d,IdkConfig *kits) {
    for(int part=0;part<7;part++) {
        d->parts[part]=-1;
        for(int kit=0;kit<kits->count;kit++) if(!kits->instances[kit].disable && kits->instances[kit].type==part+d->gender*7) {
            d->parts[part]=kit;break;
        }
    }
}
void client_design_init(ClientDesign *design,IdkConfig *kits) {
    memset(design,0,sizeof(*design));design_validate(design,kits);design->initialized=true;
}
static ActionResult design_button(ClientActions *ctx,int code) {
    ClientDesign *d=ctx->design;IdkConfig *kits=ctx->idkits;
    if(!d || !kits || !kits->count) return ACTION_FAILED;
    if(!d->initialized) client_design_init(d,kits);
    if(code>=300 && code<=313) {
        int part=(code-300)/2,kit=d->parts[part];
        if(kit>=0) for(int attempt=0;attempt<kits->count;attempt++) {
            kit=(kit+((code&1)?1:kits->count-1))%kits->count;
            if(!kits->instances[kit].disable && kits->instances[kit].type==part+d->gender*7) {d->parts[part]=kit;break;}
        }
    } else if(code>=314 && code<=323) {
        const int counts[5]={12,16,16,6,8};int part=(code-314)/2;
        d->colors[part]=(d->colors[part]+((code&1)?1:counts[part]-1))%counts[part];
    } else if(code==324 || code==325) {
        int gender=code-324;if(d->gender!=gender) {d->gender=gender;design_validate(d,kits);}
    } else if(code==326) {
        if(ctx->live && !protocol_send_design(ctx->session,d->gender,d->parts,d->colors)) return ACTION_FAILED;
        return ctx->live?ACTION_SENT:ACTION_LOCAL;
    }
    return ACTION_LOCAL;
}
void client_design_prepare(ClientActions *ctx,Component *root,Arena *frame,int cycle) {
    if(!ctx || !ctx->design || !root) return;
    if(root->client_code==327) {
        if(!ctx->design->initialized) client_design_init(ctx->design,ctx->idkits);
        PlayerEntity preview;player_entity_init(&preview);preview.visible=true;preview.gender=ctx->design->gender;
        const int slots[7]={8,11,4,6,9,7,10};
        for(int i=0;i<7;i++) if(ctx->design->parts[i]>=0) preview.appearances[slots[i]]=256+ctx->design->parts[i];
        memcpy(preview.colors,ctx->design->colors,sizeof(preview.colors));
        root->model=player_entity_get_model(&preview,ctx->idkits,ctx->objects,NULL,frame);
        root->xan=150;root->yan=((int)(sin(cycle/40.0)*256))&2047;
    }
    if(root->child_id) for(int i=0;i<root->child_count;i++) {
        Component *child=component_get(ctx->components,root->child_id[i]);
        if(child) client_design_prepare(ctx,child,frame,cycle);
    }
}

// Original Client.tryMove(type=2) is queued before the operation packet.
// A failed local approach does not fabricate movement or prevent the server
// from performing its own authoritative reach/target validation.
static bool approach(ClientActions *ctx,const PathTarget *target) {
    if(!ctx->pathfinder || !ctx->collision || !ctx->map || !ctx->entities) return false;
    PathingEntity *player=&ctx->entities->local_player.base;PathResult path;
    int plane=ctx->entities->current_plane;if(plane<0 || plane>=4) return false;
    bool found=pathfinder_find_target(ctx->pathfinder,&ctx->collision[plane],player->path_tile_x[0],player->path_tile_z[0],target,false,&path);    if(!found && target->shape==-1 && target->width==0 && target->length==0) {        PathTarget adjacent=*target;adjacent.width=adjacent.length=1;        found=pathfinder_find_target(ctx->pathfinder,&ctx->collision[plane],player->path_tile_x[0],player->path_tile_z[0],&adjacent,false,&path);    }    if(found && path.step_count>0) {        bool sent=protocol_send_operation_route(ctx->session,ctx->map->base_tile_x,ctx->map->base_tile_z,player->path_tile_x[0],player->path_tile_z[0],path.steps_x,path.steps_z,path.step_count,false);        // Original tryMove sets the same flag for scene and operation routes.        if(sent && ctx->destination_x && ctx->destination_z) {            *ctx->destination_x=path.steps_x[path.step_count-1];            *ctx->destination_z=path.steps_z[path.step_count-1];        }        return sent;    }
    return true;
}
static bool world_operation(ClientActions *ctx,MenuAction action) {
    if(!ctx->entities || !ctx->live) return false;
    PathTarget target={.shape=-1};    if(action.type==MENU_OBJ_OP && ctx->scene && ctx->map) {        if(action.x<0 || action.z<0 || action.x>=ctx->scene->max_tile_x || action.z>=ctx->scene->max_tile_z) return false;        GroundItem *item=ctx->scene->tiles[ctx->entities->current_plane][action.x][action.z].items;        while(item && item->id!=action.target) item=item->next;        if(!item) return false;        target.x=action.x;target.z=action.z;        if(!approach(ctx,&target)) return false;        return protocol_send_opobj(ctx->session,action.operation,ctx->map->base_tile_x+action.x,ctx->map->base_tile_z+action.z,action.target);    }
    if(action.type==MENU_NPC_OP) {
        if(action.target<0 || action.target>=MAX_NPCS || !ctx->entities->npcs[action.target]) return false;
        NpcEntity *npc=ctx->entities->npcs[action.target];
        target.x=npc->base.path_tile_x[0];target.z=npc->base.path_tile_z[0];target.width=target.length=1;
        if(!approach(ctx,&target)) return false;
        return protocol_send_opnpc(ctx->session,action.operation,action.target);
    }
    if(action.type==MENU_PLAYER_OP) {
        if(action.target<0 || action.target>=MAX_PLAYERS || !ctx->entities->players[action.target] || !ctx->entities->players[action.target]->visible) return false;
        PlayerEntity *player=ctx->entities->players[action.target];
        target.x=player->base.path_tile_x[0];target.z=player->base.path_tile_z[0];target.width=target.length=1;
        if(!approach(ctx,&target)) return false;
        return protocol_send_opplayer(ctx->session,action.operation,action.target);
    }
    if(action.type==MENU_LOC_OP && ctx->scene && ctx->locations && ctx->map) {
        const WorldMapLocation *found=NULL;
        for(size_t i=0;i<ctx->scene->map_location_count;i++) {
            const WorldMapLocation *loc=&ctx->scene->map_locations[i];
            if(loc->level==ctx->entities->current_plane && loc->id==action.target && loc->x==action.x && loc->z==action.z) {found=loc;break;}
        }
        if(!found) return false;
        target.x=found->x;target.z=found->z;target.rotation=found->rotation;
        if(found->shape==10 || found->shape==11 || found->shape==22) {
            LocType loc;if(!loc_get(ctx->locations,found->id,&loc)) return false;
            target.width=(found->rotation&1) ? loc.length:loc.width;
            target.length=(found->rotation&1) ? loc.width:loc.length;
            int force=loc.forceapproach,r=found->rotation;
            target.forceapproach=r ? ((force<<r)&15)+(force>>(4-r)):force;
            loc_free_entry(&loc);
        } else target.shape=found->shape;
        if(!approach(ctx,&target)) return false;
        return protocol_send_oploc(ctx->session,action.operation,ctx->map->base_tile_x+action.x,ctx->map->base_tile_z+action.z,action.target);
    }
    return false;
}

ActionResult client_action_dispatch(ClientActions *ctx,MenuAction action) {
    if(!ctx) return ACTION_FAILED;
    bool sent=false;
    if(action.type==MENU_CANCEL) {ctx->selection->item_selected=false;return ACTION_LOCAL;}
    if(action.type==MENU_WALK) return ACTION_WALK;
    if(action.type==MENU_ITEM_SELECT) {
        ObjType object;
        if(!obj_get(ctx->objects,action.target,&object)) return ACTION_FAILED;
        ctx->selection->item_selected=true;ctx->selection->item=action.target;
        ctx->selection->slot=action.slot;ctx->selection->component=action.component;
        snprintf(ctx->selection->name,sizeof(ctx->selection->name),"%s",object.name);
        obj_free_entry(&object);return ACTION_LOCAL;
    }
    if(action.type==MENU_ITEM_EXAMINE || action.type==MENU_OBJ_EXAMINE) {
        ObjType object;
        if(!obj_get(ctx->objects,action.target,&object)) return ACTION_FAILED;
        chatbox_add_message(ctx->chatbox,0,NULL,object.desc[0]?object.desc:object.name);
        obj_free_entry(&object);return ACTION_LOCAL;
    }
    if(action.type==MENU_NPC_EXAMINE && ctx->entities) {
        if(action.target<0 || action.target>=MAX_NPCS || !ctx->entities->npcs[action.target]) return ACTION_FAILED;
        NpcEntity *npc=ctx->entities->npcs[action.target];if(!npc->has_type) return ACTION_FAILED;
        chatbox_add_message(ctx->chatbox,0,NULL,npc->type.desc[0]?npc->type.desc:npc->type.name);return ACTION_LOCAL;
    }
    if(action.type==MENU_LOC_EXAMINE && ctx->locations) {
        LocType loc;if(!loc_get(ctx->locations,action.target,&loc)) return ACTION_FAILED;
        chatbox_add_message(ctx->chatbox,0,NULL,loc.desc[0]?loc.desc:loc.name);loc_free_entry(&loc);return ACTION_LOCAL;
    }
    if(action.type==MENU_NPC_OP || action.type==MENU_LOC_OP || action.type==MENU_PLAYER_OP || action.type==MENU_OBJ_OP) {
        sent=world_operation(ctx,action);
    } else if(action.type==MENU_IF_BUTTON) {
        Component *button=component_get(ctx->components,action.component);
        if(!button) return ACTION_FAILED;
        if(button->client_code>=300 && button->client_code<=326) {
            ActionResult design=design_button(ctx,button->client_code);
            if(design==ACTION_FAILED || button->client_code!=326) return design;
        }
        if(ctx->live) sent=protocol_send_button(ctx->session,action.component);
        if(!ctx->live || sent) {
            if(action.operation==4 || action.operation==5) varp_handle_component_click(ctx->varps,button);
            if(button->text && button->text[0] && (button->layer == 4262 || !ctx->live)) {
                char song_name[64];
                size_t si = 0;
                for (const char *p = button->text; *p && si < sizeof(song_name) - 1; p++) {
                    if (*p == ' ') song_name[si++] = '_';
                    else song_name[si++] = (char)tolower((unsigned char)*p);
                }
                song_name[si] = '\0';
                midi_play(song_name, 0, 0);
            }
        }
    } else if(action.type==MENU_IF_CLOSE) {
        if(ctx->live) sent=protocol_send_close_modal(ctx->session);
        if(!ctx->live || sent) {            *ctx->viewport_interface=-1;*ctx->chat_interface=-1;            if(ctx->sidebar_interface) *ctx->sidebar_interface=-1;        }
    } else if(action.type==MENU_IF_CONTINUE) {
        if(ctx->live) sent=protocol_send_resume_pausebutton(ctx->session,action.component);
        // The server controls what appears next; do not fabricate/hide a dialog.
    } else if(action.type==MENU_HELD_OP || action.type==MENU_INV_BUTTON || action.type==MENU_ITEM_ON_ITEM) {
        Component *inventory=component_get(ctx->components,action.component);
        if(!inventory || action.slot<0 || action.slot>=inventory->width*inventory->height ||
           !inventory->inv_slot_obj_id || inventory->inv_slot_obj_id[action.slot]!=action.target+1) return ACTION_FAILED;
        if(ctx->live) {
            if(action.type==MENU_HELD_OP) sent=protocol_send_opheld(ctx->session,action.operation,action.target,action.slot,action.component);
            else if(action.type==MENU_INV_BUTTON) sent=protocol_send_inv_button(ctx->session,action.operation,action.target,action.slot,action.component);
            else if(ctx->selection->item_selected) sent=protocol_send_item_on_item(ctx->session,
                action.target,action.slot,action.component,ctx->selection->item,ctx->selection->slot,ctx->selection->component);
        }
    } else return ACTION_UNAVAILABLE;
    if(ctx->trace) printf("[ACTION] %s type=%d op=%d target=%d component=%d slot=%d\n",
        sent?"sent":ctx->live?"failed":"offline",action.type,action.operation,action.target,action.component,action.slot);
    if(sent) {ctx->selection->item_selected=false;return ACTION_SENT;}
    return ctx->live ? ACTION_FAILED : ACTION_LOCAL;
}
