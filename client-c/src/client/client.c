#include "client/client.h"

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/arena.h"
#include "core/session_log.h"
#include "core/jagfile.h"
#include "core/packet.h"
#include "core/bzip2.h"
#include "core/jstring.h"
#include "core/jrandom.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/model.h"
#include "gfx/anim.h"
#include "gfx/pixfont.h"
#include "gfx/media.h"
#include "ui/component.h"
#include "ui/component_packets.h"
#include "ui/client_actions.h"
#include "ui/context_menu.h"
#include "ui/title_screen.h"
#include "config/flo.h"
#include "config/seq.h"
#include "config/loc.h"
#include "config/npc.h"
#include "config/obj.h"
#include "config/idk.h"
#include "config/varp_type.h"
#include "dash3d/collision_map.h"
#include "dash3d/world.h"
#include "dash3d/world3d.h"
#include "dash3d/minimap.h"
#include "dash3d/pathfinder.h"
#include "dash3d/map_streamer.h"
#include "dash3d/camera.h"
#include "dash3d/effects.h"
#include "ui/combat_feedback.h"
#include "audio/wave.h"
#include "audio/midi.h"
#include "ui/chatbox.h"
#include "ui/overhead_chat.h"
#include "ui/mouse_picking.h"
#include "ui/varp.h"
#include "entity/pathing_entity.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "net/entity_sync.h"
#include "net/zone_sync.h"
#include "dash3d/location_anim.h"
#include "dash3d/location_merge.h"
#include "entity/actor_model.h"
#include "net/client_stream.h"
#include "net/protocol.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "net/rsa.h"

#define VIEWPORT_X 8
#define VIEWPORT_Y 11
#define VIEWPORT_W 512
#define VIEWPORT_H 334
#ifndef RS2_SCENE225_RUNTIME
#define RS2_SCENE225_RUNTIME 0
#endif
#if RS2_SCENE225_RUNTIME
#include "dash3d/scene_runtime225.h"
static SceneRuntime225 *scene_runtime;
static uint32_t scene_pixels[VIEWPORT_W*VIEWPORT_H];
#endif

static uint32_t compute_pack_crc(const char *name) {
    char path[256];
    snprintf(path, sizeof(path), "../engine/data/pack/client/%s", name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(path, sizeof(path), "engine/data/pack/client/%s", name);
        f = fopen(path, "rb");
    }
    if (!f) {
        snprintf(path, sizeof(path), "../../Server-main/engine/data/pack/client/%s", name);
        f = fopen(path, "rb");
    }
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *b = (uint8_t*)malloc(sz);
    if (!b) { fclose(f); return 0; }
    fread(b, 1, sz, f);
    fclose(f);
    uint32_t crc = packet_crc32(b, (size_t)sz);
    free(b);
    return crc;
}

static uint8_t* decompress_map(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../../Server-main/%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../../../Server-main/%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 4) { fclose(f); return NULL; }
    uint8_t* comp = (uint8_t*)malloc(sz);
    fread(comp, 1, sz, f);
    fclose(f);

    Packet pkt = packet_wrap(comp, (size_t)sz);
    uint32_t uncomp_len = g4(&pkt);
    uint8_t* uncomp = (uint8_t*)malloc(uncomp_len);

    bzip_decompress((int8_t*)uncomp, (int8_t*)comp, (int)sz - 4, 4);
    free(comp);

    *out_len = (size_t)uncomp_len;
    return uncomp;
}

static int current_entity_frame(PathingEntity *entity,SeqConfig *sequences) {
    int id=entity->secondary_seq_id,frame=entity->secondary_seq_frame;
    if(entity->primary_seq_id>=0 && entity->primary_seq_delay==0) {id=entity->primary_seq_id;frame=entity->primary_seq_frame;}
    if(id<0 || id>=sequences->count) return -1;
    SeqType *seq=&sequences->instances[id];
    return frame>=0 && frame<seq->frame_count ? seq->frames[frame] : -1;
}

static bool project_from_point(int vx, int vy, int vz,
                               int eye_x, int eye_y, int eye_z,
                               int pitch, int yaw,
                               int *out_sx, int *out_sy) {
    int dx = vx - eye_x;
    int dy = vy - eye_y;
    int dz = vz - eye_z;

    int sin_p = pix3d_sin_table[pitch & 2047];
    int cos_p = pix3d_cos_table[pitch & 2047];
    int sin_y = pix3d_sin_table[yaw & 2047];
    int cos_y = pix3d_cos_table[yaw & 2047];

    int x1 = (dx * cos_y + dz * sin_y) >> 16;
    int z1 = (dz * cos_y - dx * sin_y) >> 16;

    int y2 = (dy * cos_p - z1 * sin_p) >> 16;
    int z2 = (dy * sin_p + z1 * cos_p) >> 16;

    if (z2 < 50) {
        return false;
    }

    *out_sx = pix3d_center_x + (x1 << 9) / z2;
    *out_sy = pix3d_center_y + (y2 << 9) / z2;
    return true;
}

// ---------------------------------------------------------------------------
// Client state. The original keeps these as fields of its single client
// instance; one client runs per process here too.

static ClientPlatform plat;
static const char *capture_path;
static bool offline, trace;
static bool capture_clicks;
static int last_captured_cross = -1, last_cross_cycle = 400;
static int capture_frame, capture_interval, server_port, capture_yaw;
static int rendered_frames;
// client.updateGame's closing packets: heartbeatTimer (instance) and the static cyclelogic4.
static int heartbeat_timer, cyclelogic4;
// client.java anticheat state. Login rerolls the offsets and zoom; the modifiers and
// offset cycles are never reset, and the cyclelogic counters are static.
static int camera_offset_cycle, minimap_offset_cycle, camera_moved_write;
static int camera_offset_x_modifier = 2, camera_offset_z_modifier = 2, camera_offset_yaw_modifier = 1;
static int minimap_anticheat_angle, minimap_zoom, minimap_angle_modifier = 2, minimap_zoom_modifier = 1;
static int cyclelogic1, cyclelogic2, cyclelogic3, cyclelogic5, cyclelogic6;
// GameShell.idleCycles and mouseClickButton (held until the next game cycle consumes it).
static int idle_cycles, cycle_click_button;
// REBUILD_NORMAL sets sceneState 1; the next PLAYER_INFO builds the scene.
static void client_entity_movement(PathingEntity *entity);
#if !RS2_SCENE225_RUNTIME
static ActionResult client_dispatch_action(MenuAction action);
#endif
static bool scene_build_pending;
#if RS2_SCENE225_RUNTIME
static uint32_t loading_viewport225[512*334];
static uint32_t loading_mapback225[168*160];
static bool loading_mapback_valid225;
typedef struct {int frame,cycle;Model *saved;} InterfacePose225;
static InterfacePose225 *interface_pose225;
static int interface_draw_cycle225;
static uint32_t chat_pixels225[479*96];
static void client_interface_reset225(Component *root);
static void client_loading_message225(bool updated,bool retain_scene);
#endif

static int build_old_base_x, build_old_base_z;

static Arena arena_perm, arena_frame;
static uint32_t *pixels;
static Jagfile *title_jag;
static bool has_anim;
static FloConfig flo_cfg;
static SeqConfig seq_cfg;
static SpotAnimConfig spot_cfg;
static LocConfig loc_cfg;
static NpcConfig npc_cfg;
static ObjConfig obj_cfg;
static IdkConfig idk_cfg;
static PixFont font_p11, font_p12, font_b12, font_q8;
static bool has_fonts;
static Media media;
static bool has_media;
static ComponentConfig comp_cfg;
static bool has_components;

static const int DEFAULT_TAB_INTERFACE_ID[14] = {
    2423, // 0: Combat
    3917, // 1: Stats
    638,  // 2: Quest list
    3213, // 3: Inventory
    1644, // 4: Worn items
    5608, // 5: Prayer
    1151, // 6: Magic
    -1,   // 7: Unused
    5065, // 8: Friends
    5715, // 9: Ignore
    2449, // 10: Logout
    904,  // 11: Game options
    149,  // 12: Player controls
    962   // 13: Music player
};
static int tab_interface_id[14] = {
    2423, 3917, 638, 3213, 1644, 5608, 1151, -1, 5065, 5715, 2449, 904, 149, 962
};
static int sticky_chat_interface_id = -1;
#if RS2_SCENE225_RUNTIME
static char modal_message225[256];
#endif
static int flashing_tab = -1;
static bool dev_tabs_unlocked = false;

// Hint Arrow state (Packet 25)
static int hint_type = 0;       // 0 = none, 1 = npc, 2 = tile/coord, 10 = player
static int hint_npc = -1;
static int hint_tile_x = 0;
static int hint_tile_z = 0;
static int hint_offset_x = 0;
static int hint_offset_z = 0;
static int hint_height = 0;
static int hint_player = -1;

static World *world;
static CollisionMap collision_maps[4];
static void client_build_minimap(int level);
static World3D *world3d;
static Minimap minimap;
static bool has_minimap;
static SceneEffects effects;
static ZoneSync zones;
static EntitySyncContext sync_ctx;
static PlayerEntity *player_ptr;
#define player (*player_ptr)
static NpcEntity hans;
static Camera cam;
static int selected_tab = 3;   // inventory tab by default
static InterfaceState interfaces;
static int public_chat_setting = 0;  // 0: On, 1: Friends, 2: Off, 3: Hide
static int private_chat_setting = 0; // 0: On, 1: Friends, 2: Off
static int trade_chat_setting = 0;   // 0: On, 1: Friends, 2: Off
static ContextMenu menu;
static MousePicker picker;
static Pathfinder pathfinder;
static MapStreamer streamer;
static InputTracking input_tracking;
static LastLoginInfo225 last_login225;
#ifdef _WIN32
typedef struct {
    HANDLE thread;
    volatile LONG references,done;
    uint32_t address;
    char dns[256];
} LoginDns225;
static LoginDns225 *login_dns225;
static uint64_t login_dns_started225;
static void login_dns_release225(LoginDns225 *job){if(InterlockedDecrement(&job->references)==0)free(job);}
static DWORD WINAPI login_dns_worker225(LPVOID argument){
    LoginDns225 *job=argument;struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(job->address);
    if(getnameinfo((struct sockaddr*)&address,sizeof(address),job->dns,sizeof(job->dns),NULL,0,0))strcpy(job->dns,"unknown");
    InterlockedExchange(&job->done,1);login_dns_release225(job);return 0;
}
static void client_login_dns_clear225(void){
    if(login_dns225){CloseHandle(login_dns225->thread);login_dns_release225(login_dns225);login_dns225=NULL;}
    login_dns_started225=0;
}
static void client_login_dns_poll225(void){
    if(login_dns225 && InterlockedCompareExchange(&login_dns225->done,0,0)){
        /* signlink.run publishes the old result then clears dnsreq, even if
         * dnslookup replaced the pending request while the lookup was busy. */
        snprintf(last_login225.dns,sizeof(last_login225.dns),"%s",login_dns225->dns);
        login_dns_started225=last_login225.dns_generation;
        CloseHandle(login_dns225->thread);login_dns_release225(login_dns225);login_dns225=NULL;
    }
    if(!login_dns225 && last_login225.dns_generation!=login_dns_started225){
        login_dns_started225=last_login225.dns_generation;
        LoginDns225 *job=calloc(1,sizeof(*job));if(!job)return;
        job->references=2;unsigned a,b,c,d;
        if(sscanf(last_login225.dns_request,"%u.%u.%u.%u",&a,&b,&c,&d)!=4){free(job);return;}
        job->address=a<<24|b<<16|c<<8|d;
        job->thread=CreateThread(NULL,0,login_dns_worker225,job,0,NULL);
        if(!job->thread){free(job);return;}login_dns225=job;
    }
}
#else
static void client_login_dns_clear225(void){}
static void client_login_dns_poll225(void){}
#endif
static VarpContext varp_ctx;
/* Shared clientcode metadata also drives split private chat in legacy builds. */
static VarpTypeConfig texture_varp_types;
#if RS2_PIX3D_REV225_TEXTURE
static void texture_varp_brightness(int value) {
    double base;
    if(value==1) base=0.9;
    else if(value==2) base=0.8;
    else if(value==3) base=0.7;
    else if(value==4) base=0.6;
    else return;
#if RS2_SCENE225_RUNTIME
    double effective=base+math_random()*0.03-0.015;
#else
    double effective=pix3d225_preview_brightness(base);
#endif
    pix3d_init_colour_table(effective);
    pix3d225_texture_cache_set_brightness(effective);
#if RS2_SCENE225_RUNTIME
    pix3d_texture_palette_brightness225(effective);
#endif
}
#endif
static int mouse_x = 0, mouse_y = 0;
static ClickFeedback click_feedback;
static bool pending_click = false;
#if RS2_SCENE225_RUNTIME
static void client_walk_result225(void);
static ActionResult client_dispatch_action(MenuAction action);
static bool client_override_chat225(void);
static bool client_location_present225(void *user,uint32_t bits);
static bool world_action_pending225;
static MenuAction world_action225;
static int world_action_x225,world_action_y225;
static int pending_pick_x225,pending_pick_y225;
#endif
static int pending_click_x = 0, pending_click_y = 0;
static bool running = true;
static GameSession session;
static bool live_server = false;
// client.ingame: false shows the title screen (updateTitle/drawTitleScreen).
static bool ingame;
static bool login_pending = false, reconnecting = false, logout_now = false;
static int system_update_timer = 0; /* client.systemUpdateTimer, units of 1/50 second */
static int idle_timeout = 0;   // client.idleTimeout
static TitleScreen title;
static Chatbox chatbox;
static WordFilterTables chat_filter;
static bool private_override_chat225;
#if RS2_SCENE225_RUNTIME
static OverheadChat overhead_chat225;
static int scene_draw_cycle225;
#endif
static void client_player_chat(void *user,PlayerEntity *actor,bool packed,int colour,int rights,const uint8_t *text,size_t length) {
    (void)user;
    if(packed) {
        if(!chatbox_add_public_message(&chatbox,&chat_filter,actor,text,length,colour,rights,private_override_chat225))
            fprintf(stderr,"[CHAT] Public message decode/filter failed\n");
    } else chatbox_add_message(&chatbox,2,actor->name,(const char*)text);
}

static MenuSelection selection;
static ClientDesign design;
static ClientActions actions;
static int title_click_button = 0, title_click_x = 0, title_click_y = 0;
static int game_ticks = 0;
static int location_anim_cycle = 0;
static ClientHeldKeys held;
static bool capture_this_frame;
static char interval_path[1024];
static const char *frame_capture_path;

// ---------------------------------------------------------------------------
// Input queue. Events reach the client in the order the SDL queue delivered
// them: pointers resolved while drawing, then scripted input, then OS input.

#define CLIENT_EVENT_QUEUE 1024
static ClientEvent event_queue[CLIENT_EVENT_QUEUE];
#if RS2_SCENE225_RUNTIME
static ClientEvent key_events225[CLIENT_EVENT_QUEUE];
static int key_event_count225;
/* GameShell records the latest press separately from the current held button. */
static ClientEvent mouse_press225;
static int mouse_button225;
static bool menu_opened_cycle225, minimap_click_pending225;
static int minimap_click_x225,minimap_click_y225;
#endif
static int event_read, event_write;

static int64_t client_tracking_time(void){
    uint64_t bits=plat.tracking_now_ms?plat.tracking_now_ms():(plat.now_ms?plat.now_ms():0);int64_t now;memcpy(&now,&bits,sizeof(now));return now;
}
static bool client_tracking_event(const ClientEvent *event){
    int kind=0,code=0;
    bool only=event->type>=CLIENT_EVENT_TRACK_KEY_DOWN && event->type<=CLIENT_EVENT_MOUSE_EXIT;
    if(!input_tracking.enabled)return only;
    switch(event->type){
        case CLIENT_EVENT_MOUSE_MOVE:kind=TRACK_MOVE;break;
        case CLIENT_EVENT_MOUSE_DOWN:kind=TRACK_PRESS;code=event->button==CLIENT_BUTTON_RIGHT;break;
        case CLIENT_EVENT_MOUSE_UP:kind=TRACK_RELEASE;code=event->button==CLIENT_BUTTON_RIGHT;break;
        case CLIENT_EVENT_TRACK_KEY_DOWN:kind=TRACK_KEY_PRESS;code=event->key;break;
        case CLIENT_EVENT_TRACK_KEY_UP:kind=TRACK_KEY_RELEASE;code=event->key;break;
        case CLIENT_EVENT_FOCUS_GAIN:kind=TRACK_FOCUS_GAIN;break;
        case CLIENT_EVENT_FOCUS_LOSS:kind=TRACK_FOCUS_LOSS;break;
        case CLIENT_EVENT_MOUSE_ENTER:kind=TRACK_ENTER;break;
        case CLIENT_EVENT_MOUSE_EXIT:kind=TRACK_EXIT;break;
        default:return false;
    }
    if((kind==TRACK_MOVE || kind==TRACK_PRESS) && (event->x<0 || event->x>=789 || event->y<0 || event->y>=532))return only;
    if(!input_tracking_event(&input_tracking,kind,event->x,event->y,code,client_tracking_time()))fprintf(stderr,"[TRACK] Invalid recorder state\n");
    return only;
}

void client_queue_event(const ClientEvent *event) {
    if(!event || client_tracking_event(event))return;
    int next = (event_write + 1) % CLIENT_EVENT_QUEUE;
    if (next == event_read) { fprintf(stderr, "[INPUT] event queue full; event dropped\n"); return; }
    event_queue[event_write] = *event;
    event_write = next;
}

void client_set_held_keys(const ClientHeldKeys *keys) {
    // A held-key change is a GameShell keyPressed/keyReleased, which resets idleCycles.
    if (keys->left != held.left || keys->right != held.right || keys->up != held.up || keys->down != held.down ||
        keys->w != held.w || keys->a != held.a || keys->s != held.s || keys->d != held.d || keys->ctrl != held.ctrl) idle_cycles = 0;
    held = *keys;
}

static int client_name_compare(const char *a, const char *b) {
    for (;; a++, b++) {
        int x = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a, y = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (x != y || !x) return x - y;
    }
}

// The SDL key names input scripts use for the keys the client handles.
ClientKey client_key_from_name(const char *name) {
    static const struct { const char *name; ClientKey key; } names[] = {
        {"Escape", CLIENT_KEY_ESCAPE}, {"F1", CLIENT_KEY_F1}, {"F2", CLIENT_KEY_F2}, {"F3", CLIENT_KEY_F3},
        {"F4", CLIENT_KEY_F4}, {"F5", CLIENT_KEY_F5}, {"F6", CLIENT_KEY_F6}, {"F7", CLIENT_KEY_F7},
        {"F8", CLIENT_KEY_F8}, {"F9", CLIENT_KEY_F9}, {"F10", CLIENT_KEY_F10}, {"F11", CLIENT_KEY_F11},
        {"F12", CLIENT_KEY_F12}, {"Backspace", CLIENT_KEY_BACKSPACE}, {"Delete", CLIENT_KEY_DELETE},
        {"Tab", CLIENT_KEY_TAB}, {"Return", CLIENT_KEY_RETURN}, {"Keypad Enter", CLIENT_KEY_KP_ENTER}
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (!client_name_compare(name, names[i].name)) return names[i].key;
    return CLIENT_KEY_NONE;
}

// ---------------------------------------------------------------------------
// Optional replay of scripted input. It never edits inventory, varps or server state.

typedef struct { int frame; char kind[16]; char value[256]; } VerificationInput;
static VerificationInput verification_input[512];
static int verification_input_count, verification_input_next;
static bool verification_follow_input;
static char verification_input_path[1024];
static long verification_input_offset;
static int verification_npc_type=-1,verification_cursor_x=-1,verification_cursor_y=-1;
static int verification_obj_id=-1,verification_inventory_slot=-1;
static int verification_loc_id=-1;
static char verification_option[64];
static bool verification_load_input(const char *path) {
    FILE *file=fopen(path,"r"); if(!file) return false;
    if(strlen(path)>=sizeof(verification_input_path)) {fclose(file);return false;}
    if(path!=verification_input_path) strcpy(verification_input_path,path);
    if(verification_input_offset && fseek(file,verification_input_offset,SEEK_SET)) {fclose(file);return false;}
    char line[320];int previous=verification_input_count?verification_input[verification_input_count-1].frame:-1;
    while(fgets(line,sizeof(line),file)) {
        if(line[0]=='#' || line[0]=='\n' || line[0]=='\r') continue;
        if(verification_input_count>=512) {fclose(file);return false;}
        VerificationInput *input=&verification_input[verification_input_count];
        if(sscanf(line,"%d %15s %255[^\r\n]",&input->frame,input->kind,input->value)!=3 || input->frame<previous) {fclose(file);return false;}
        if(strcmp(input->kind,"move") && strcmp(input->kind,"loc") && strcmp(input->kind,"npc") && strcmp(input->kind,"obj") && strcmp(input->kind,"inv") && strcmp(input->kind,"option") && strcmp(input->kind,"right_cursor") && strcmp(input->kind,"left_cursor") && strcmp(input->kind,"quit") && strcmp(input->kind,"left") && strcmp(input->kind,"release") && strcmp(input->kind,"right") && strcmp(input->kind,"text") && strcmp(input->kind,"key")) {fclose(file);return false;}
        previous=input->frame;verification_input_count++;
    }
    verification_input_offset=ftell(file);fclose(file);return true;
}
static void verification_push_input(int frame) {
    if(verification_follow_input && frame%10==0 && verification_input_next==verification_input_count &&
        !verification_load_input(verification_input_path)) {
        fprintf(stderr,"Invalid appended verification input; follow disabled\n");verification_follow_input=false;
    }
    while(verification_input_next<verification_input_count && verification_input[verification_input_next].frame<frame) {
        fprintf(stderr,"Skipped late verification event at frame %d (current %d)\n",verification_input[verification_input_next].frame,frame);
        verification_input_next++;
    }
    while(verification_input_next<verification_input_count && verification_input[verification_input_next].frame==frame) {
        VerificationInput *input=&verification_input[verification_input_next++];ClientEvent event;memset(&event,0,sizeof(event));
        printf("[INPUT] frame=%d event=%s\n",frame,input->kind);
        if(!strcmp(input->kind,"loc")) {
            verification_loc_id=atoi(input->value);verification_obj_id=verification_inventory_slot=verification_npc_type=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"npc")) {
            verification_loc_id=-1;
            verification_npc_type=atoi(input->value);verification_obj_id=verification_inventory_slot=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"obj")) {
            verification_loc_id=-1;
            verification_obj_id=atoi(input->value);verification_npc_type=verification_inventory_slot=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"inv")) {
            verification_loc_id=-1;
            verification_inventory_slot=atoi(input->value);verification_npc_type=verification_obj_id=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"option")) {
            snprintf(verification_option,sizeof(verification_option),"%.63s",input->value);
        } else if(!strcmp(input->kind,"quit")) {
            event.type=CLIENT_EVENT_QUIT;client_queue_event(&event);
        } else if(!strcmp(input->kind,"right_cursor") || !strcmp(input->kind,"left_cursor")) {
            if(verification_cursor_x>=0) {
                event.type=CLIENT_EVENT_MOUSE_DOWN;event.button=!strcmp(input->kind,"right_cursor")?CLIENT_BUTTON_RIGHT:CLIENT_BUTTON_LEFT;
                event.x=verification_cursor_x;event.y=verification_cursor_y;client_queue_event(&event);
            } else fprintf(stderr,"[INPUT] no resolved cursor for %s at frame %d\n",input->kind,frame);
        } else if(!strcmp(input->kind,"key")) {
            event.type=CLIENT_EVENT_KEY_DOWN;event.key=client_key_from_name(input->value);client_queue_event(&event);
        } else if(!strcmp(input->kind,"text")) {
            const char *text=input->value;while(*text) {
                event.type=CLIENT_EVENT_TEXT;size_t size=strlen(text);if(size>31) size=31;
                memcpy(event.text,text,size);event.text[size]=0;text+=size;client_queue_event(&event);
            }
        } else {
            int x,y;if(sscanf(input->value,"%d %d",&x,&y)!=2) continue;
            if(!strcmp(input->kind,"move")) {event.type=CLIENT_EVENT_MOUSE_MOVE;event.x=x;event.y=y;}
            else {event.type=!strcmp(input->kind,"release")?CLIENT_EVENT_MOUSE_UP:CLIENT_EVENT_MOUSE_DOWN;event.x=x;event.y=y;event.button=!strcmp(input->kind,"right")?CLIENT_BUTTON_RIGHT:CLIENT_BUTTON_LEFT;}
            client_queue_event(&event);
        }
    }
}

// client.logout(): close the stream, return to title state 0 with empty
// credentials, clear model caches, reset the scene and collision maps and
// stop the music. The title artwork is loaded again by drawTitleScreen.
static void client_logout(void) {
    input_tracking_disable(&input_tracking);
    protocol_close_session(&session);
    live_server = false; ingame = false; reconnecting = false; login_pending = false;
#if RS2_SCENE225_RUNTIME
    mouse_button225=cycle_click_button=0;key_event_count225=0;
#endif
    actions.live = false;
    map_streamer_clear(&streamer);scene_build_pending=false;
    title_screen_logout_reset(&title);
    loc_clear_model_cache(true); actor_models_clear();
    world3d_reset(world3d);
    for (int level = 0; level < 4; level++) collision_map_reset(&collision_maps[level]);
    music_logout225(&varp_ctx.music);
    midi_stop();
    if (!title_screen_load(&title, title_jag)) { fprintf(stderr, "Title screen reload failed\n"); running = false; }
    printf("[STATE] logout -> title state=%d\n", title.state);
}

// ---------------------------------------------------------------------------
// Options

void client_default_options(ClientOptions *options) {
    memset(options, 0, sizeof(*options));
    options->capture_frame = 10;
    options->server_port = 43594;
    options->walk_x = -1; options->walk_z = -1;
    options->capture_pitch = 128;
}

bool client_parse_args(int argc, char **argv, ClientOptions *o) {
    client_default_options(o);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--offline") == 0) o->offline = true;
        else if (strcmp(argv[i], "--trace") == 0) o->trace=true;
        else if (strcmp(argv[i], "--input-script") == 0 && i+1<argc) o->input_path=argv[++i];
        else if (strcmp(argv[i], "--input-follow") == 0) o->follow_input=true;
        else if (strcmp(argv[i], "--rebuild") == 0) o->capture_rebuild = true;
        else if (strcmp(argv[i], "--capture-clicks") == 0) o->capture_clicks = true;
        else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) o->capture_path = argv[++i];
        else if (strcmp(argv[i], "--capture-frame") == 0 && i + 1 < argc) o->capture_frame = atoi(argv[++i]);
        else if (strcmp(argv[i], "--capture-interval") == 0 && i + 1 < argc) o->capture_interval = atoi(argv[++i]);
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) o->server_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--walk-to") == 0 && i + 2 < argc) { o->walk_x = atoi(argv[++i]); o->walk_z = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--camera-yaw") == 0 && i + 1 < argc) o->capture_yaw = atoi(argv[++i]) & 2047;
        else if (strcmp(argv[i], "--camera-pitch") == 0 && i + 1 < argc) o->capture_pitch = atoi(argv[++i]);
        // Verification determinism: --seed fixes Math.random(), --fixed-step runs
        // exactly one 50 Hz cycle per rendered frame instead of wall-clock pacing.
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) { o->has_seed = true; o->seed = strtoll(argv[++i], NULL, 10); }
        else if (strcmp(argv[i], "--fixed-step") == 0) o->fixed_step = true;
        else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) o->record_path = argv[++i];
        else { fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]); return false; }
    }
    if (o->capture_interval < 0 || ((o->capture_interval || o->capture_clicks) && !o->capture_path) || o->capture_frame < 1 || o->capture_pitch < 128 || o->capture_pitch > 383 || o->server_port < 1 || o->server_port > 65535) {
        fprintf(stderr, "Invalid capture frame, camera pitch (128..383), or server port (1..65535)\n"); return false;
    }
    if ((o->capture_rebuild || o->walk_x >= 0 || o->walk_z >= 0) && (!o->capture_path || !o->offline)) {
        fprintf(stderr, "Scripted rebuild/walk requires --offline and --capture\n"); return false;
    }
    if (o->record_path && o->follow_input) {
        // Appended input is read at wall-clock times a replay cannot reproduce.
        fprintf(stderr, "--record cannot be combined with --input-follow\n"); return false;
    }
    o->login_username = getenv("RS2_USERNAME");
    o->login_password = getenv("RS2_PASSWORD");
    return true;
}

// ---------------------------------------------------------------------------
// client.load

#if RS2_SCENE225_RUNTIME
static uint32_t scene225_hash_values(const int32_t *values,size_t count){
    uint32_t hash=1;for(size_t i=0;i<count;i++)hash=hash*31+(uint32_t)values[i];return hash;
}
static uint32_t scene225_tint(uint32_t rgb,int r,int g,int b,int minimum){
    int values[3]={(int)(rgb>>16&255)+r,(int)(rgb>>8&255)+g,(int)(rgb&255)+b};
    for(int i=0;i<3;i++){if(values[i]<minimum)values[i]=minimum;if(values[i]>255)values[i]=255;}
    return (uint32_t)(values[0]<<16|values[1]<<8|values[2]);
}
static void scene225_translate_media(Media *art){
    int r=(int)(math_random()*21.0)-10,g=(int)(math_random()*21.0)-10,b=(int)(math_random()*21.0)-10;
    int common=(int)(math_random()*41.0)-20;r+=common;g+=common;b+=common;
    for(int i=0;i<50;i++){
        Pix32 *function=art->mapfunction[i];Pix8 *scene=art->mapscene[i];
        if(function)for(int j=0;j<function->width*function->height;j++)
            if(function->pixels[j])function->pixels[j]=scene225_tint(function->pixels[j],r,g,b,1);
        if(scene)for(int j=0;j<scene->palette_count;j++)scene->palette[j]=scene225_tint(scene->palette[j],r,g,b,0);
    }
}
#endif

bool client_init(const ClientOptions *options, const ClientPlatform *platform) {
    input_tracking_init(&input_tracking);
    audio_queue_cold225();
    client_login_dns_clear225();memset(&last_login225,0,sizeof(last_login225));
    plat = *platform;
    private_override_chat225=false;
#if RS2_SCENE225_RUNTIME
    memset(&overhead_chat225,0,sizeof(overhead_chat225));scene_draw_cycle225=0;
#endif
    if (options->has_seed) math_random_seed(options->seed);
    capture_path = options->capture_path;
    offline = options->offline;
    trace = options->trace;
    capture_clicks = options->capture_clicks;
    capture_frame = options->capture_frame;
    capture_interval = options->capture_interval;
    server_port = options->server_port;
    capture_yaw = options->capture_yaw;
    verification_follow_input = options->follow_input;
    bool autologin = options->login_username && *options->login_username && options->login_password && *options->login_password;
    rendered_frames = 0;
    ingame = offline;
#if RS2_SCENE225_RUNTIME
    if(!offline)for(int i=0;i<14;i++)tab_interface_id[i]=-1;
    memset(loading_viewport225,0,sizeof(loading_viewport225));
#endif
#if RS2_SCENE225_RUNTIME
    /* World class initialization precedes client.load; local archives replace
     * the successful CRC URL read, whose nonce also consumes Math.random. */
    int initial_hue225=(int)(math_random()*17.0)-8;
    int initial_light225=(int)(math_random()*33.0)-16;
    (void)math_random();
    title_screen_init(&title);
#endif

    if(options->input_path && (!capture_path || !verification_load_input(options->input_path))) {
        fprintf(stderr,"Invalid input script; --capture is required\n");return false;
    }
    printf("[Lost City 2004 C] Starting RS2 Revision 225 Client...\n");

    arena_perm = arena_create(48 * 1024 * 1024);

    pixels = (uint32_t *)calloc(CLIENT_WIDTH * CLIENT_HEIGHT, sizeof(uint32_t));

    // 1. Initialize Pix3D Engine
    pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
    pix3d_init();
#if RS2_SCENE225_RUNTIME
    model_use_preview225(true);
#endif
    /* protocol_login_ex sends low-memory 0, matching client.setHighMemory.
     * Keep texture dimensions and terrain texturing in that same mode. */
#if RS2_SCENE225_RUNTIME
    pix3d_low_detail = false;
#endif
    title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena_perm);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena_perm);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena_perm);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena_perm);

    has_fonts = false;
    if (title_jag) {
        pixfont_load(&font_p11, title_jag, "p11", &arena_perm);
        pixfont_load(&font_p12, title_jag, "p12", &arena_perm);
        pixfont_load(&font_b12, title_jag, "b12", &arena_perm);
        pixfont_load(&font_q8,  title_jag, "q8",  &arena_perm);
        has_fonts = true;
        printf("[PASS] 2004scape Bitmap Fonts loaded (p11, p12, b12, q8)\n");
    }

#if RS2_SCENE225_RUNTIME
    if (!title_jag || !has_fonts || !title_screen_load(&title, title_jag)) {
        fprintf(stderr,"Title screen cache artwork could not be loaded\n"); return false;
    }
#endif

    has_media = false;
    if (media_jag) {
        has_media = media_load(&media, media_jag, &arena_perm);
        if (has_media) {
            printf("[PASS] 2004scape Media artwork loaded (frames, tabs, compass, icons)\n");
        }
    }

#if RS2_SCENE225_RUNTIME
    /* client.load translates map art before setBrightness and config decode. */
    scene225_translate_media(&media);
    if(trace)printf("[BOOT225] stage=media random_state=%llu\n",(unsigned long long)math_random_state());
#endif
    double texture_brightness = 0.8;
#if RS2_PIX3D_REV225_TEXTURE
    // Sample Java's brightness jitter while preserving the established
    // protocol RNG sequence until the full startup random stream is ported.
#if RS2_SCENE225_RUNTIME
    texture_brightness += math_random()*0.03-0.015;
    if(trace)printf("[BOOT225] stage=brightness effective=%.17g random_state=%llu\n",texture_brightness,(unsigned long long)math_random_state());
#else
    texture_brightness = pix3d225_preview_brightness(texture_brightness);
#endif
    pix3d225_texture_cache_init(pix3d_low_detail);
#endif
    pix3d_init_colour_table(texture_brightness);
    pix3d_init_2d();

    // 2. Load Cache Archives
    printf("[INFO] Loading 2004scape cache...\n");
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena_perm);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena_perm);
    if (!config_jag) {
        fprintf(stderr, "Error: Could not locate config cache archive!\n");
        return false;
    }
    Jagfile *wordenc_jag=jagfile_load_file("../engine/data/pack/client/wordenc",&arena_perm);
    if(!wordenc_jag)wordenc_jag=jagfile_load_file("engine/data/pack/client/wordenc",&arena_perm);
    if(!wordenc_jag || !wordfilter_tables_load(&chat_filter,wordenc_jag)) {
        fprintf(stderr,"Error: Could not load revision225 wordenc filter!\n");return false;
    }
    varp_type_free(&texture_varp_types);
    if(!varp_type_unpack(&texture_varp_types,config_jag)) return false;

    Jagfile *textures_jag = jagfile_load_file("../engine/data/pack/client/textures", &arena_perm);
    if (!textures_jag) textures_jag = jagfile_load_file("engine/data/pack/client/textures", &arena_perm);
    int texture_count=0;
    for (int id=0;textures_jag && id<50;id++) {
        char name[8]; snprintf(name,sizeof(name),"%d",id);
        Pix8 *image=pix8_load(textures_jag,name,0,NULL);
        if(!image) continue;
        if (pix3d_low_detail && image->crop_w == 128) pix8_shrink(image,NULL);
        else pix8_crop(image,NULL);
#if RS2_PIX3D_REV225_TEXTURE
        pix3d225_texture_cache_set_image(id,image);
#if RS2_SCENE225_RUNTIME
        pix3d_set_texture_palette_gamma225(id,image->palette,image->palette_count,texture_brightness);
#else
        pix3d_set_texture_palette_gamma(id,image->palette,image->palette_count,texture_brightness);
#endif
#else
        pix3d_set_texture_palette(id,image->palette,image->palette_count);
#endif
        uint32_t *rgb=malloc((size_t)image->width*image->height*sizeof(uint32_t));
        if(rgb) {
            for(int i=0;i<image->width*image->height;i++) {
                uint32_t col=image->palette[image->pixels[i]], out=0;
#if RS2_PIX3D_REV225_TEXTURE
                out=pix3d_gamma_correct(col,texture_brightness)&0xF8F8FFu;
#else
                if(image->pixels[i]) {
                    for(int shift=0;shift<=16;shift+=8) out|=(uint32_t)(pow(((col>>shift)&255)/256.0,0.8)*256.0)<<shift;
                    if(!out) out=1;
                }
#endif
                rgb[i]=out;
            }
            pix3d_set_texture(id,rgb,image->width,image->height);
            free(rgb); texture_count++;
        }
        pix8_free(image);
    }
#if RS2_PIX3D_REV225_TEXTURE
    pix3d225_texture_cache_set_brightness(texture_brightness);
#endif
    printf("[Textures] Loaded %d cache textures\n",texture_count);

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena_perm);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena_perm);
    if (!models_jag) {
        fprintf(stderr, "Error: Could not locate models cache archive!\n");
        return false;
    }

    model_init_repo(models_jag, &arena_perm);

    Jagfile *sounds_jag = jagfile_load_file("../engine/data/pack/client/sounds", &arena_perm);
    if (!sounds_jag) sounds_jag = jagfile_load_file("engine/data/pack/client/sounds", &arena_perm);
    if (sounds_jag && !RS2_SCENE225_RUNTIME) {
        wave_unpack(sounds_jag);
        printf("[PASS] Audio synthesizer online (sounds.dat unpacked)\n");
    }

    // client.load: AnimBase/AnimFrame precede SeqType, whose delays resolve from frames.
    has_anim = anim_unpack(models_jag, &arena_perm);
    if (has_anim) {
        printf("[PASS] %d Animation Bases & %d Frames loaded from models cache\n",
               g_anim_cfg.base_count, g_anim_cfg.frame_count);
    }

    flo_unpack(&flo_cfg, config_jag);
    seq_unpack(&seq_cfg, config_jag);
    if(!spotanim_unpack(&spot_cfg,config_jag,&seq_cfg)) {fprintf(stderr,"Spot animation cache decode failed\n");return false;}
    loc_unpack(&loc_cfg, config_jag);
    npc_unpack(&npc_cfg, config_jag);
    obj_unpack(&obj_cfg, config_jag, false);
    idk_unpack(&idk_cfg, config_jag, &arena_perm);
#if RS2_SCENE225_RUNTIME
    if(trace)printf("[BOOT225] stage=config random_state=%llu\n",(unsigned long long)math_random_state());
    if(sounds_jag) {
        tone_init225();
        if(!wave_unpack(sounds_jag))return false;
        printf("[PASS] Audio synthesizer online (original shared noise initialization)\n");
    }
    if(trace)printf("[BOOT225] stage=sounds random_state=%llu\n",(unsigned long long)math_random_state());
    if(trace){
        uint32_t floors=1;for(int i=0;i<flo_cfg.count;i++)floors=floors*31+(uint32_t)flo_cfg.instances[i].hsl;
        printf("[BOOT225] palette=%08x floors=%08x hue=%d light=%d\n",scene225_hash_values(pix3d_colour_table,65536),floors,initial_hue225,initial_light225);
    }
#endif

    printf("[INFO] Cache loaded: %d flo, %d seq, %d loc, %d npc, %d obj, %d idk\n",
           flo_cfg.count, seq_cfg.count, loc_cfg.count, npc_cfg.count, obj_cfg.count, idk_cfg.count);

    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena_perm);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena_perm);

    has_components = false;
    PixFont *font_ptrs[4] = { &font_p11, &font_p12, &font_b12, &font_q8 };
    if (interface_jag && has_fonts && media_jag) {
        has_components = component_unpack(&comp_cfg, media_jag, font_ptrs, interface_jag, &arena_perm);
        if (has_components) {
            comp_cfg.obj_config = &obj_cfg;
            comp_cfg.font_p11 = &font_p11;
            comp_cfg.scrollbar0 = media.scrollbar0;
            comp_cfg.scrollbar1 = media.scrollbar1;
            printf("[PASS] %d UI Components unpacked from interface.jag\n", comp_cfg.count);

            // Populate starter inventory into Component 3214 (4x7 grid inside tab 3)
            Component *inv_grid = component_get(&comp_cfg, 3214);
            if (offline && inv_grid && inv_grid->inv_slot_obj_id) {
                inv_grid->inv_slot_obj_id[0] = 996; // Coins
                inv_grid->inv_slot_obj_count[0] = 25000;
                inv_grid->inv_slot_obj_id[1] = 1278; // Bronze sword
                inv_grid->inv_slot_obj_count[1] = 1;
                inv_grid->inv_slot_obj_id[2] = 1172; // Wooden shield
                inv_grid->inv_slot_obj_count[2] = 1;
                inv_grid->inv_slot_obj_id[3] = 380; // Lobster
                inv_grid->inv_slot_obj_count[3] = 15;
                inv_grid->inv_slot_obj_id[4] = 560; // Death rune
                inv_grid->inv_slot_obj_count[4] = 100;
                inv_grid->inv_slot_obj_id[5] = 555; // Fire rune
                inv_grid->inv_slot_obj_count[5] = 250;
            }
        }
    }

    // 3. Build Lumbridge 3D Scene
    // content/maps/labels.txt locates Lumbridge at (3239, 3233), in mapsquare 50_50.
    printf("[INFO] Building Lumbridge 3D scene (m50_50 / l50_50)...\n");
    const char *m_path = "../engine/data/pack/client/maps/m50_50";
    const char *l_path = "../engine/data/pack/client/maps/l50_50";
    size_t m_len = 0, l_len = 0;
    uint8_t *m_data = decompress_map(m_path, &m_len);
    if (!m_data) {
        m_path = "engine/data/pack/client/maps/m50_50";
        m_data = decompress_map(m_path, &m_len);
    }
    uint8_t *l_data = decompress_map(l_path, &l_len);
    if (!l_data) {
        l_path = "engine/data/pack/client/maps/l50_50";
        l_data = decompress_map(l_path, &l_len);
    }
    if (!m_data || !l_data) {
        fprintf(stderr, "Lumbridge map cache (m50_50/l50_50) is missing\n");
        free(m_data); free(l_data); return false;
    }

    world = (World*)calloc(1, sizeof(World));
    world_init(world, 104, 104);
#if RS2_SCENE225_RUNTIME
    world->build225=true;
    world->random_hue_offset=initial_hue225;
    world->random_lightness_offset=initial_light225;
#endif

    for (int i = 0; i < 4; i++) collision_map_init(&collision_maps[i], 104, 104);

    bool preview_scene225 = offline || !RS2_SCENE225_RUNTIME;
    if (m_data && preview_scene225) {
        world_load_ground(world, m_data, m_len, 0, 0, 50 * 64, 50 * 64);
    }
    free(m_data);

    world3d = (World3D*)calloc(1, sizeof(World3D));
    world3d_init(world3d, 104, 104, 4);
    world3d->sequences=&seq_cfg;
    world3d->trace_location_animations=trace;

    if (l_data && preview_scene225) {
        world_load_locations(world, l_data, l_len, &loc_cfg, world3d, collision_maps, 0, 0);
    }
    free(l_data);

    if(preview_scene225)world_build(world, world3d, &flo_cfg, collision_maps);
    loc_clear_model_cache(false);
    printf("[PASS] 3D Scene built successfully!\n");

    // Initialize Minimap Radar & Compass Needle
    has_minimap = false;
    if (has_media) {
        minimap_init(&minimap, &media, &arena_perm);
        if(preview_scene225)client_build_minimap(0);
        has_minimap = true;
        printf("[PASS] Minimap radar & compass needle initialized\n");
    }

    // 4. Spawn Player Character (Lumbridge Courtyard at Tile 32, 32)
    effects.config=&spot_cfg;effects.trace=trace;
    zones.effects=&effects;zones.trace=trace;zones.objects=&obj_cfg;zones.local_pid=-1;
    entity_sync_init(&sync_ctx, &npc_cfg);sync_ctx.trace=trace;
    sync_ctx.seq_cfg=&seq_cfg;
    zones.entities=&sync_ctx;
    player_ptr = &sync_ctx.local_player;

    player_entity_init(&player);
    pathing_entity_teleport(&player.base, false, false, 32, 32);

    // Default Rev 225 appearance
    player.gender = 0;
    int default_app[12] = { 256 + 0, 0, 0, 0, 256 + 18, 0, 256 + 26, 256 + 36, 0, 256 + 33, 256 + 42, 256 + 10 };
    memcpy(player.appearances, default_app, sizeof(default_app));
    player.colors[1] = 2; // blue torso
    player.colors[2] = 2; // blue legs
    strncpy(player.name, "Player", sizeof(player.name));
    player.combat_level = 3;
    player.visible = true;

    // 5. Spawn NPC Hans at Tile 34, 34
    npc_entity_init(&hans);
    if (npc_get(&npc_cfg, 0, &hans.type)) {
        hans.has_type = true;
        pathing_entity_teleport(&hans.base, false, false, 34, 34);
        hans.base.seq_stand_id = 808;
        hans.base.secondary_seq_id = 808;
    }

    player.base.seq_stand_id = 808;
    player.base.seq_walk_id = 819;
    player.base.seq_turn_id = 823;
    player.base.seq_turn_left_id = 821;
    player.base.seq_turn_right_id = 822;
    player.base.seq_turn_around_id = 820;
    player.base.secondary_seq_id = 808;

    arena_frame = arena_create(4 * 1024 * 1024);

    // 6. Camera Settings (authentic Rev 225 orbit system)
    camera_init(&cam, player.base.x, player.base.z);
    cam.orbit_yaw = capture_yaw;
    cam.orbit_pitch = options->capture_pitch;
    interface_state_init(&interfaces);

    // Pre-configure NPC Dialogue 4882 (npcchat1) with Hans's head model and greeting
    Component *diag_4882 = component_get(&comp_cfg, 4882);
    if (diag_4882) {
        Component *c_head = component_get(&comp_cfg, 4883);
        if (c_head) {
            Model *head_m = npc_get_head_model(&hans.type, &arena_perm);
            if (!head_m) {
                head_m = npc_get_model(&hans.type, &arena_perm);
            }
            c_head->model = head_m;
        }
        Component *c_name = component_get(&comp_cfg, 4884);
        if (c_name) {
            c_name->text = "Hans";
        }
        Component *c_text = component_get(&comp_cfg, 4885);
        if (c_text) {
            c_text->text = "Hello adventurer! Welcome to Lumbridge Castle.";
        }
    }

    context_menu_init(&menu);
    mouse_picker_init(&picker);
    world3d->picker=&picker;
    pathfinder_init(&pathfinder);
    map_streamer_clear(&streamer);
    streamer.base_tile_x = 3200;
    streamer.base_tile_z = 3200;
    if (options->capture_rebuild) {
        // Replay a local packet-237 payload through the production decoder/rebuilder.
        // This verifies rebuilding with the real cache without requiring a server.
        uint8_t rebuild_data[14] = {0};
        Packet rebuild_packet = packet_wrap(rebuild_data, sizeof(rebuild_data));
        p2(&rebuild_packet, 406); p2(&rebuild_packet, 406);
        p1(&rebuild_packet, 50); p1(&rebuild_packet, 50);
        p4(&rebuild_packet, 0); p4(&rebuild_packet, 0);
        rebuild_packet.pos = 0;
        if (!map_streamer_parse_rebuild_packet(&streamer, &rebuild_packet) ||
            !map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, &loc_cfg,
                                 "../engine/data/pack/client/maps")) {
            fprintf(stderr, "Capture map rebuild failed\n"); return false;
        }
        if (has_minimap) client_build_minimap(0);
        printf("[VERIFY] Packet-237 map rebuild completed\n");
    }
    if (options->walk_x >= 0 && options->walk_z >= 0) {
        PathResult path;
        if (!pathfinder_find_path(&pathfinder, &collision_maps[0], player.base.path_tile_x[0],
                                 player.base.path_tile_z[0], options->walk_x, options->walk_z, false, &path)) {
            fprintf(stderr, "Capture walk target is unreachable\n"); return false;
        }
        pathing_entity_queue_path(&player.base, path.steps_x, path.steps_z, path.step_count, false);
    }
    varp_init(&varp_ctx);
    varp_ctx.types=&texture_varp_types;
    audio_set_wave_volume225(0);
    comp_cfg.varp_ctx = &varp_ctx;

    printf("[INFO] Entering main render loop. Controls:\n");
    printf("       [Arrow Keys] Rotate camera Pitch & Yaw\n");
    printf("       [W, A, S, D] Move only in explicit offline preview\n");
    printf("       [Left Click] Walk to tile / Interact / Select tab\n");
    printf("       [Right Click] Open available target operations\n");
    printf("       [ESC] Close active interface/menu, otherwise exit\n");

    // Initialize Network Session (attempt connection to local 2004scape server)
    client_stream_global_init();
    protocol_init_session(&session);

    const char *pack_names[8] = {
        "title", "config", "interface", "media", "models", "textures", "wordenc", "sounds"
    };
    uint32_t crcs[9];
    crcs[0] = 0;
    for (int i = 0; i < 8; i++) {
        crcs[i + 1] = compute_pack_crc(pack_names[i]);
    }
    protocol_set_crcs(&session, crcs);

    if(!RS2_SCENE225_RUNTIME)title_screen_init(&title);
    if (offline) {
        if(RS2_SCENE225_RUNTIME)title_screen_unload(&title);
        printf("[MODE] Explicit offline demonstration; inventory/entities are fixtures, no gameplay verification.\n");
    } else {
        if (!title_jag || !has_fonts || !title_screen_load(&title, title_jag)) {
            fprintf(stderr, "Title screen cache artwork could not be loaded\n"); return false;
        }
        printf("[STATE] title screen state=%d\n", title.state);
        if (autologin) {
            // Launcher convenience: fill the original login box and press Login.
            snprintf(title.username, sizeof(title.username), "%s", options->login_username);
            snprintf(title.password, sizeof(title.password), "%s", options->login_password);
            title.state = 2; title.login_field = 0;
            title.message0[0] = '\0';
            snprintf(title.message1, sizeof(title.message1), "Connecting to server...");
            login_pending = true;
        }
    }

    chatbox_init(&chatbox, "Player");
    if (!live_server && offline) {
        chatbox_add_message(&chatbox,0,NULL,"Offline scene preview. Gameplay requires a server.");
    }
    comp_cfg.selection=&selection;comp_cfg.chatbox=&chatbox;
    picker.selection=&selection;
    actions=(ClientActions){.session=&session,.components=&comp_cfg,.varps=&varp_ctx,.objects=&obj_cfg,
        .selection=&selection,.chatbox=&chatbox,.viewport_interface=&interfaces.viewport,
        .chat_interface=&interfaces.chat,.sidebar_interface=&interfaces.sidebar,.live=live_server,.trace=trace,.idkits=&idk_cfg,.design=&design,
        .entities=&sync_ctx,.scene=world3d,.locations=&loc_cfg,.pathfinder=&pathfinder,.collision=collision_maps,.map=&streamer,
        .destination_x=has_minimap ? &minimap.flag_scene_tile_x : NULL,
        .destination_z=has_minimap ? &minimap.flag_scene_tile_z : NULL,
        .click_feedback=&click_feedback,.selected_tab=&selected_tab,.idle_timeout=&idle_timeout};
    return true;
}

// ---------------------------------------------------------------------------
// Frame entry points

void client_begin_frame(void) {
    midi_is_playing();
    verification_push_input(rendered_frames);
}

#if RS2_SCENE225_RUNTIME
/* handleMinimapInput follows actor updates and precedes updateOrbitCamera. */
static void client_minimap_route225(int mx,int my) {
            if (live_server && (RS2_SCENE225_RUNTIME || (interfaces.viewport == -1 && !comp_cfg.drag.active))) {
                int yaw=cam.orbit_yaw&2047;
                MinimapClick click;
                PathResult route;
                if(minimap_click_target(mx,my,player.base.x,player.base.z,yaw+minimap_anticheat_angle,minimap_zoom,&click) &&
                    pathfinder_find_path(&pathfinder,&collision_maps[sync_ctx.current_plane],
                    player.base.path_tile_x[0],player.base.path_tile_z[0],click.tile_x,click.tile_z,true,&route) && route.step_count>0) {
                    if(trace) printf("[NAV] click=(%d,%d) yaw=%d source=(%d,%d) target=(%d,%d) nearest=%d steps=%d\n",
                        mx,my,yaw,streamer.base_tile_x+player.base.path_tile_x[0],streamer.base_tile_z+player.base.path_tile_z[0],
                        streamer.base_tile_x+click.tile_x,streamer.base_tile_z+click.tile_z,route.reached_nearest,route.step_count);
                    bool ctrl=held.ctrl;
                    bool sent=protocol_send_minimap_route(&session,streamer.base_tile_x,streamer.base_tile_z,
                        player.base.path_tile_x[0],player.base.path_tile_z[0],route.steps_x,route.steps_z,route.step_count,
                        ctrl,click.click_x,click.click_y,yaw,minimap_anticheat_angle,minimap_zoom,player.base.x,player.base.z,route.reached_nearest);
                    if(sent) {
                        minimap.flag_scene_tile_x=route.steps_x[route.step_count-1];
                        minimap.flag_scene_tile_z=route.steps_z[route.step_count-1];
                    } else fprintf(stderr,"[ACTION] Minimap route send failed\n");
                    if(trace) printf("[ACTION] minimap route %s destination=(%d,%d)\n",sent?"sent":"failed",
                        streamer.base_tile_x+route.steps_x[route.step_count-1],streamer.base_tile_z+route.steps_z[route.step_count-1]);
                }
            }

}
#endif

static void client_handle_event(const ClientEvent *event) {
// GameShell listeners: every mouse and key event resets idleCycles; a press sets mouseClickButton.
if (event->type == CLIENT_EVENT_MOUSE_MOVE || event->type == CLIENT_EVENT_MOUSE_DOWN || event->type == CLIENT_EVENT_MOUSE_UP ||
    event->type == CLIENT_EVENT_KEY_DOWN || event->type == CLIENT_EVENT_KEY_UP || event->type == CLIENT_EVENT_TEXT) idle_cycles = 0;
if (event->type == CLIENT_EVENT_MOUSE_DOWN) cycle_click_button = event->button == CLIENT_BUTTON_RIGHT ? 2 : 1;
if (!ingame && event->type != CLIENT_EVENT_QUIT) {
    if (event->type == CLIENT_EVENT_MOUSE_MOVE) {
        mouse_x = event->x; mouse_y = event->y;
    } else if (event->type == CLIENT_EVENT_MOUSE_DOWN) {
        mouse_x = event->x; mouse_y = event->y;
        title_click_x = mouse_x; title_click_y = mouse_y;
        title_click_button = event->button == CLIENT_BUTTON_RIGHT ? 2 : 1;
    } else if (event->type == CLIENT_EVENT_TEXT) {
        const unsigned char *text = (const unsigned char *)event->text;
        for (int i = 0; text[i]; i++) {
            if (text[i] == 0xC2 && text[i + 1] == 0xA3) { title_screen_push_key(&title, 163); i++; }
            else if (text[i] >= 30 && text[i] < 128) title_screen_push_key(&title, text[i]);
        }
    } else if (event->type == CLIENT_EVENT_KEY_DOWN) {
        int key = event->key;
        if (key == CLIENT_KEY_BACKSPACE || key == CLIENT_KEY_DELETE) title_screen_push_key(&title, 8);
        else if (key == CLIENT_KEY_TAB) title_screen_push_key(&title, 9);
        else if (key == CLIENT_KEY_RETURN || key == CLIENT_KEY_KP_ENTER) title_screen_push_key(&title, 10);
    }
    return;
}
if (event->type == CLIENT_EVENT_QUIT) {
    running = false;
} else if (event->type == CLIENT_EVENT_TEXT) {
    if(interfaces.viewport!=-1 && interfaces.viewport==chatbox.report_interface){for(int i=0;event->text[i];i++)chatbox_report_key(&chatbox,interfaces.viewport,(uint8_t)event->text[i]);return;}
    if(live_server && interfaces.chat!=-1 && !chatbox.count_dialog_open && !chatbox.private_prompt_open)return;
    for (int i = 0; event->text[i] != '\0'; i++) {
        chatbox_key_press(&chatbox, event->text[i]);
    }
} else if (event->type == CLIENT_EVENT_KEY_DOWN) {
    if(interfaces.viewport!=-1 && interfaces.viewport==chatbox.report_interface && (event->key==CLIENT_KEY_BACKSPACE || event->key==CLIENT_KEY_DELETE || event->key==CLIENT_KEY_RETURN || event->key==CLIENT_KEY_KP_ENTER)){chatbox_report_key(&chatbox,interfaces.viewport,(event->key==CLIENT_KEY_BACKSPACE || event->key==CLIENT_KEY_DELETE)?8:13);return;}
    // GameShell.keyPressed has no Escape mapping, and client.java never reads the
    // F1-F12 key codes (1008+), so neither closes interfaces nor switches tabs in Rev 225.
    if(live_server && interfaces.chat!=-1 && !chatbox.count_dialog_open && !chatbox.private_prompt_open &&
        (event->key==CLIENT_KEY_BACKSPACE || event->key==CLIENT_KEY_DELETE || event->key==CLIENT_KEY_RETURN || event->key==CLIENT_KEY_KP_ENTER))return;
    if (event->key == CLIENT_KEY_BACKSPACE || event->key == CLIENT_KEY_DELETE) {
        chatbox_backspace(&chatbox);
    } else if (event->key == CLIENT_KEY_RETURN || event->key == CLIENT_KEY_KP_ENTER) {
        if(chatbox.private_prompt_open) {
            chatbox.public_mode=public_chat_setting;chatbox.private_mode=private_chat_setting;chatbox.trade_mode=trade_chat_setting;
            if(chatbox.social_action==3?chatbox_send_private(&chatbox,&chat_filter,&session):chatbox_submit_social(&chatbox,&session,player.name))private_chat_setting=chatbox.private_mode;
            else {fprintf(stderr,"[CHAT] Private send failed; prompt retained\n");if(session.stream.out_overflow)reconnecting=true;}
            return;
        }
        if (chatbox.count_dialog_open) {
            int count = chatbox_count_value(&chatbox);
            if (live_server) {
                    bool sent=protocol_send_resume_p_countdialog(&session, count);
                    if(trace) printf("[ACTION] quantity %s count=%d\n",sent?"sent":"failed",count);
            }
            chatbox_close_count_dialog(&chatbox);
            return;
        }
        if(live_server && chatbox.input_len>0 && !(chatbox.input[0]==':' && chatbox.input[1]==':')) {
            chatbox.public_mode=public_chat_setting;chatbox.private_mode=private_chat_setting;chatbox.trade_mode=trade_chat_setting;
            if(chatbox_send_public(&chatbox,&chat_filter,&player,&session))public_chat_setting=chatbox.public_mode;
            else {fprintf(stderr,"[CHAT] Public send failed; input retained\n");if(session.stream.out_overflow)reconnecting=true;}
            return;
        }
        const char *cmd = live_server ? chatbox_take_input(&chatbox):chatbox_submit(&chatbox);
        if (cmd) {
            if (live_server && cmd[0] == ':' && cmd[1] == ':') {
                bool sent=protocol_send_command(&session,cmd+2);
                if(trace) printf("[ACTION] command %s bytes=%zu\n",sent?"sent":"failed",strlen(cmd+2));
            }
            if(live_server) {
                return;
            }
            if (strncmp(cmd, "::coords", 8) == 0) {
                char coords_msg[64];
                snprintf(coords_msg, sizeof(coords_msg), "Coordinates: (%d, %d)", player.base.x / 128, player.base.z / 128);
                chatbox_add_message(&chatbox, 0, NULL, coords_msg);
            } else if (strncmp(cmd, "::tabs", 6) == 0 || strncmp(cmd, "::unlocktabs", 11) == 0) {
                dev_tabs_unlocked = true;
                for (int t = 0; t < 14; t++) {
                    tab_interface_id[t] = DEFAULT_TAB_INTERFACE_ID[t];
                }
                chatbox_add_message(&chatbox, 0, NULL, "@yel@All 14 tabs forced active & unlocked.");
            } else if (strncmp(cmd, "::mainland", 10) == 0 || strncmp(cmd, "::lumby", 7) == 0 || strncmp(cmd, "::skip", 6) == 0) {
                dev_tabs_unlocked = true;
                for (int t = 0; t < 14; t++) {
                    tab_interface_id[t] = DEFAULT_TAB_INTERFACE_ID[t];
                }
                if (live_server) {
                    protocol_send_command(&session, "home");
                }
                chatbox_add_message(&chatbox, 0, NULL, "@yel@Teleporting to Mainland Lumbridge. All tabs unlocked.");
            } else if (strncmp(cmd, "::music", 7) == 0 || strncmp(cmd, "::songs", 7) == 0) {
                selected_tab = 13;
                tab_interface_id[13] = 962;
                for (int m = 20; m <= 25; m++) varp_ctx.varps[m] = -1;
                chatbox_add_message(&chatbox, 0, NULL, "@yel@Music tab active (interface 962). All 175 tracks unlocked.");
                if (live_server) protocol_send_command(&session, "unlockalltracks");
            } else if (strncmp(cmd, "::unlock", 8) == 0) {
                for (int m = 20; m <= 25; m++) varp_ctx.varps[m] = -1;
                chatbox_add_message(&chatbox, 0, NULL, "@yel@All music tracks unlocked.");
                if (live_server) protocol_send_command(&session, "unlockalltracks");
            } else if (strncmp(cmd, "::song ", 7) == 0) {
                const char *song = cmd + 7;
                while (*song == ' ') song++;
                if (*song) {
                    bool started = midi_play(song, 0, 0);
                    char song_msg[96];
                    snprintf(song_msg, sizeof(song_msg), started ? "@yel@Now playing: %s" : "@red@Could not play: %s", song);
                    chatbox_add_message(&chatbox, 0, NULL, song_msg);
                }
            } else if (strncmp(cmd, "::stopsong", 10) == 0 || strncmp(cmd, "::stopmusic", 11) == 0) {
                midi_stop();
                chatbox_add_message(&chatbox, 0, NULL, "@yel@Music stopped.");
            } else if (strncmp(cmd, "::bank", 6) == 0) {
                interfaces.viewport = 5292;
                chatbox_add_message(&chatbox, 0, NULL, "@yel@Opened Bank interface (5292).");
            } else if (strncmp(cmd, "::shop", 6) == 0) {
                interfaces.viewport = 300;
                chatbox_add_message(&chatbox, 0, NULL, "@yel@Opened Shop interface (300).");
            } else if (strncmp(cmd, "::talk", 6) == 0) {
                interfaces.chat = 4882;
            } else if (strncmp(cmd, "::close", 7) == 0) {
                interfaces.viewport = -1;
                interfaces.chat = -1;
            } else if (strncmp(cmd, "::clear", 7) == 0) {
                chatbox_free(&chatbox);
                chatbox_init(&chatbox, "Player");
            }
        }
    }
} else if (event->type == CLIENT_EVENT_MOUSE_WHEEL) {
    if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
        Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
        if (modal_com) {
            int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
            int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
            int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
            int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
            component_handle_mousewheel(&comp_cfg, modal_com, mouse_x, mouse_y, mx_pos, my_pos, 0, event->wheel);
        }
    } else if (selected_tab >= 0 && selected_tab < 14) {
        int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
        if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
        if (tab_if > 0 && tab_if != 65535) {
            Component *tab_com = component_get(&comp_cfg, tab_if);
            if (tab_com) {
                component_handle_mousewheel(&comp_cfg, tab_com, mouse_x, mouse_y, 562, 231, 0, event->wheel);
            }
        }
    }
} else if (event->type == CLIENT_EVENT_MOUSE_MOVE) {
    mouse_x = event->x;
    mouse_y = event->y;
    comp_cfg.mouse_x = mouse_x;
    comp_cfg.mouse_y = mouse_y;
    if (comp_cfg.drag.active) {
#if !RS2_SCENE225_RUNTIME
        if (abs(mouse_x - comp_cfg.drag.grab_x) > 5 || abs(mouse_y - comp_cfg.drag.grab_y) > 5) {
            comp_cfg.drag.threshold = true;
        }
#endif
    }
    if (comp_cfg.scroll_drag.active && comp_cfg.scroll_drag.target && comp_cfg.scroll_drag.arrow_dir == 0) {
        Component *target = comp_cfg.scroll_drag.target;
        int h = comp_cfg.scroll_drag.h;
        int s_max = comp_cfg.scroll_drag.s_max;
        int thumb_h = (h - 32) * h / (s_max > 0 ? s_max : 1);
        if (thumb_h < 8) thumb_h = 8;
        int track_avail = h - 32 - thumb_h;
        int scroll_range = s_max - h;
        int thumb_rel = mouse_y - comp_cfg.scroll_drag.sy - 16 - comp_cfg.scroll_drag.grab_offset;
        if (thumb_rel < 0) thumb_rel = 0;
        if (thumb_rel > track_avail) thumb_rel = track_avail;
        if (track_avail > 0 && scroll_range > 0) {
            target->scroll_position = scroll_range * thumb_rel / track_avail;
        }
        if (target->scroll_position > scroll_range) target->scroll_position = scroll_range;
        if (target->scroll_position < 0) target->scroll_position = 0;
    }
} else if (event->type == CLIENT_EVENT_MOUSE_DOWN) {
    int mx = event->x;
    int my = event->y;

#if !RS2_SCENE225_RUNTIME
    mouse_x = mx; mouse_y = my;
#endif
#if RS2_SCENE225_RUNTIME
    if(event->button==CLIENT_BUTTON_LEFT && modal_message225[0]) {
        modal_message225[0]=0;cycle_click_button=0;return;
    }
#endif
    actions.click_x=mx; actions.click_y=my;
    comp_cfg.mouse_x = mouse_x; comp_cfg.mouse_y = mouse_y;
    actions.live = live_server;
    if(comp_cfg.drag.active)return; /* Original handleMouseInput defers all presses during an item drag. */
    if(!menu.visible && selection.spell_selected && mx>=520 && mx<=788 && my>=165 && my<=230)return;
    if (event->button == CLIENT_BUTTON_RIGHT) {
#if RS2_SCENE225_RUNTIME
        if (!menu.visible && has_fonts && menu.size > 0) {
            context_menu_open225(&menu,mx,my,&font_b12);menu_opened_cycle225=true;
        }
#else
        if (has_fonts && menu.size > 0) context_menu_open225(&menu, mx, my, &font_b12);
#endif
#if RS2_SCENE225_RUNTIME
        if(trace && getenv("RS2_TRACE_ACTORS"))printf("[MENU225 OPEN] frame=%d click=%d,%d hover=%d,%d rows=%d visible=%d rect=%d,%d,%d,%d\n",rendered_frames,mx,my,mouse_x,mouse_y,menu.size,menu.visible,menu.x,menu.y,menu.width,menu.height);
        if(trace && getenv("RS2_TRACE_ACTORS"))for(int i=0;i<menu.size;i++)printf("[MENU225 OPEN ROW] frame=%d row=%d label=%s\n",rendered_frames,i,menu.options[i]);
#endif
    } else if (event->button == CLIENT_BUTTON_LEFT) {
        // Original handleMinimapInput hit rectangle and inverse rotation.
        // Consume this click before stale scene/context-menu actions.
        if (!menu.visible && has_minimap && mx >= 582 && mx < 728 && my >= 14 && my < 165) {
            /* A visible menu consumes the click in handleMouseInput. A modal
             * alone does not suppress the original handleMinimapInput. */
#if RS2_SCENE225_RUNTIME
            minimap_click_pending225=true;minimap_click_x225=mx;minimap_click_y225=my;
#else
            if (live_server && (RS2_SCENE225_RUNTIME || (interfaces.viewport == -1 && !comp_cfg.drag.active))) {
                int yaw=cam.orbit_yaw&2047;
                MinimapClick click;
                PathResult route;
                if(minimap_click_target(mx,my,player.base.x,player.base.z,yaw+minimap_anticheat_angle,minimap_zoom,&click) &&
                    pathfinder_find_path(&pathfinder,&collision_maps[sync_ctx.current_plane],
                    player.base.path_tile_x[0],player.base.path_tile_z[0],click.tile_x,click.tile_z,true,&route) && route.step_count>0) {
                    if(trace) printf("[NAV] click=(%d,%d) yaw=%d source=(%d,%d) target=(%d,%d) nearest=%d steps=%d\n",
                        mx,my,yaw,streamer.base_tile_x+player.base.path_tile_x[0],streamer.base_tile_z+player.base.path_tile_z[0],
                        streamer.base_tile_x+click.tile_x,streamer.base_tile_z+click.tile_z,route.reached_nearest,route.step_count);
                    bool ctrl=held.ctrl;
                    bool sent=protocol_send_minimap_route(&session,streamer.base_tile_x,streamer.base_tile_z,
                        player.base.path_tile_x[0],player.base.path_tile_z[0],route.steps_x,route.steps_z,route.step_count,
                        ctrl,click.click_x,click.click_y,yaw,minimap_anticheat_angle,minimap_zoom,player.base.x,player.base.z,route.reached_nearest);
                    if(sent) {
                        minimap.flag_scene_tile_x=route.steps_x[route.step_count-1];
                        minimap.flag_scene_tile_z=route.steps_z[route.step_count-1];
                    } else fprintf(stderr,"[ACTION] Minimap route send failed\n");
                    if(trace) printf("[ACTION] minimap route %s destination=(%d,%d)\n",sent?"sent":"failed",
                        streamer.base_tile_x+route.steps_x[route.step_count-1],streamer.base_tile_z+route.steps_z[route.step_count-1]);
                }
            }
#endif
            return;
        }
        MenuAction action = {.type = MENU_CANCEL}; bool chosen = false;
        if (menu.visible) {
            chosen = context_menu_pick_action(&menu, mx, my, &action);
            if (chosen) {
                ActionResult result = client_dispatch_action(action);
                if (result == ACTION_WALK && interfaces.viewport == -1) {
                    pending_click = true; pending_click_x = mx; pending_click_y = my;
#if RS2_SCENE225_RUNTIME
                    pending_pick_x225=action.x;pending_pick_y225=action.z;
#endif
                } else if (result == ACTION_UNAVAILABLE) {
                    fprintf(stderr, "[ACTION] Unsupported action type %d; no packet sent\n", action.type);
                }
            }
        } else {
            if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
                Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
                if (modal_com) {
                    int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                    int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                    int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                    int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                    if (component_handle_scroll_recursive(&comp_cfg, modal_com, mx, my, mx_pos, my_pos, 0)) {
                        return;
                    }
                }
            }
            if (selected_tab >= 0 && selected_tab < 14) {
                int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
                if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
                if (tab_if > 0 && tab_if != 65535) {
                    Component *tab_com = component_get(&comp_cfg, tab_if);
                    if (tab_com) {
                        if (component_handle_scroll_recursive(&comp_cfg, tab_com, mx, my, 562, 231, 0)) {
                            return;
                        }
                    }
                }
            }
            int tab = -1;
            const int left[14] = {549, 579, 607, 635, 676, 704, 732, 550, 582, 609, 637, 679, 706, 734};
            const int right[14] = {583, 609, 637, 679, 706, 734, 766, 584, 612, 639, 681, 709, 736, 768};
            for (int i = 0; i < 14; i++) {
                bool row = i < 7 ? my >= 194 && my < 231 : my >= 492 && my < 529;
                if (row && mx >= left[i] && mx <= right[i]) {
                    if (interfaces.sidebar == -1 && (dev_tabs_unlocked || tab_interface_id[i] != -1)) {
                        tab = i;
                        break;
                    }
                }
            }
            if (tab >= 0) {
                selected_tab = tab;
                if (selected_tab == flashing_tab) {
                    flashing_tab = -1;
                }
                selection.item_selected = false;
                selection.spell_selected = false;
            } else if (mx < 520 && my >= 490 && my < 532) {
                if(mx>=416 && mx<=516 && my<=522){(void)client_report_open(&actions,"",false);}
                else if(my<=522){bool changed=true;
                    if (mx >= 8 && mx <= 108) public_chat_setting = (public_chat_setting + 1) % 4;
                    else if (mx >= 137 && mx <= 237) private_chat_setting = (private_chat_setting + 1) % 3;
                    else if (mx >= 275 && mx <= 375) trade_chat_setting = (trade_chat_setting + 1) % 3;
                    else changed=false;
                    if (live_server && changed) protocol_send_chat_setmode(&session, public_chat_setting, private_chat_setting, trade_chat_setting);
                }
            } else {
                int clicked_slot = -1;
                Component *inv_com = NULL;
                if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
                    Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
                    if (modal_com) {
                        int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                        int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                        int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                        int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                        inv_com = component_find_inv_slot(&comp_cfg, modal_com, mx, my, mx_pos, my_pos, 0, &clicked_slot);
                    }
                }
                if (!inv_com && selected_tab >= 0 && selected_tab < 14) {
                    int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
                    if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
                    if (tab_if > 0 && tab_if != 65535) {
                        Component *tab_com = component_get(&comp_cfg, tab_if);
                        if (tab_com) {
                            inv_com = component_find_inv_slot(&comp_cfg, tab_com, mx, my, 562, 231, 0, &clicked_slot);
                        }
                    }
                }
                if (!inv_com && interfaces.chat != -1 && mx >= 22 && mx < 501 && my >= 375 && my < 471) {
                    Component *chat_com = component_get(&comp_cfg, interfaces.chat);
                    if (chat_com) {
                        inv_com = component_find_inv_slot(&comp_cfg, chat_com, mx, my, 22, 375, 0, &clicked_slot);
                    }
                }

                if (inv_com && inv_com->draggable && clicked_slot >= 0 &&
                    inv_com->inv_slot_obj_id && clicked_slot < inv_com->width * inv_com->height &&
                    inv_com->inv_slot_obj_id[clicked_slot] > 0) {
                    comp_cfg.drag.active = true;
                    comp_cfg.drag.threshold = false;
                    comp_cfg.drag.cycles = 0;
                    comp_cfg.drag.component_id = inv_com->id;
                    comp_cfg.drag.slot = clicked_slot;
                    comp_cfg.drag.grab_x = mx;
                    comp_cfg.drag.grab_y = my;
                } else {
                    if(context_menu_primary_opens225(&menu,varp_ctx.mouse_buttons_option) && has_fonts) {
                        context_menu_open225(&menu,mx,my,&font_b12);
#if RS2_SCENE225_RUNTIME
                        menu_opened_cycle225=menu.visible;
#endif
                        return;
                    }
                    if (menu.size > 0) { action = menu.actions[menu.size - 1]; chosen = true; }
                    if (chosen) {
                        ActionResult result = client_dispatch_action(action);
                        if (result == ACTION_WALK && interfaces.viewport == -1) {
                            pending_click = true; pending_click_x = mx; pending_click_y = my;
#if RS2_SCENE225_RUNTIME
                            pending_pick_x225=mx;pending_pick_y225=my;
#endif
                        } else if (result == ACTION_UNAVAILABLE) {
                            fprintf(stderr, "[ACTION] Unsupported action type %d; no packet sent\n", action.type);
                        }
                    }
                }
            }
        }
    }
} else if (event->type == CLIENT_EVENT_MOUSE_UP) {
    if (event->button == CLIENT_BUTTON_LEFT && comp_cfg.scroll_drag.active) {
        comp_cfg.scroll_drag.active = false;
        comp_cfg.scroll_drag.target = NULL;
        comp_cfg.scroll_drag.arrow_dir = 0;
    }
    if (event->button == CLIENT_BUTTON_LEFT && comp_cfg.drag.active) {
        int up_x = event->x;
        int up_y = event->y;
        if (comp_cfg.drag.threshold && comp_cfg.drag.cycles >= 5) {
            int dest_slot = -1;
            Component *dest_com = NULL;
            if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
                Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
                if (modal_com) {
                    int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                    int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                    int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                    int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                    dest_com = component_find_inv_slot(&comp_cfg, modal_com, up_x, up_y, mx_pos, my_pos, 0, &dest_slot);
                }
            }
            if (!dest_com && selected_tab >= 0 && selected_tab < 14) {
                int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
                if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
                if (tab_if > 0 && tab_if != 65535) {
                    Component *tab_com = component_get(&comp_cfg, tab_if);
                    if (tab_com) {
                        dest_com = component_find_inv_slot(&comp_cfg, tab_com, up_x, up_y, 562, 231, 0, &dest_slot);
                    }
                }
            }
            if (!dest_com && interfaces.chat != -1 && up_x >= 22 && up_x < 501 && up_y >= 375 && up_y < 471) {
                Component *chat_com = component_get(&comp_cfg, interfaces.chat);
                if (chat_com) {
                    dest_com = component_find_inv_slot(&comp_cfg, chat_com, up_x, up_y, 22, 375, 0, &dest_slot);
                }
            }

            int from=comp_cfg.drag.slot;
            if (component_drag_release(&comp_cfg,dest_com,dest_slot)) {
                if (live_server) {
                    bool sent=protocol_send_inv_buttond(&session,dest_com->id,from,dest_slot);
                    if(trace) printf("[ACTION] drag %s component=%d from=%d to=%d\n",sent?"sent":"failed",dest_com->id,from,dest_slot);
                }
            }
        } else if(context_menu_primary_opens225(&menu,varp_ctx.mouse_buttons_option) && has_fonts) {
            context_menu_open225(&menu,comp_cfg.drag.grab_x,comp_cfg.drag.grab_y,&font_b12);
#if RS2_SCENE225_RUNTIME
            menu_opened_cycle225=menu.visible;
#endif
        } else {
            if (menu.size > 0) {
                MenuAction action = menu.actions[menu.size - 1];
                ActionResult result = client_dispatch_action(action);
                if (result == ACTION_WALK && interfaces.viewport == -1) {
                    pending_click = true; pending_click_x = up_x; pending_click_y = up_y;
#if RS2_SCENE225_RUNTIME
                    pending_pick_x225=up_x;pending_pick_y225=up_y;
#endif
                }
            }
        }
        comp_cfg.drag.active = false;
    }
}
}

void client_process_events(void) {
    while (event_read != event_write) {
        ClientEvent event = event_queue[event_read];
        event_read = (event_read + 1) % CLIENT_EVENT_QUEUE;
#if RS2_SCENE225_RUNTIME
        if(ingame && (event.type==CLIENT_EVENT_MOUSE_DOWN || event.type==CLIENT_EVENT_MOUSE_UP)) {
            idle_cycles=0;
            if(event.type==CLIENT_EVENT_MOUSE_DOWN) {
                mouse_press225=event;
                mouse_button225=cycle_click_button=event.button==CLIENT_BUTTON_RIGHT?2:1;
            } else mouse_button225=0;
            continue;
        }
        if(ingame && (event.type==CLIENT_EVENT_TEXT || event.type==CLIENT_EVENT_KEY_DOWN)) {
            idle_cycles=0;
            if(key_event_count225>=CLIENT_EVENT_QUEUE) {
                fprintf(stderr,"Deferred keyboard input queue exhausted\n");running=false;return;
            }
            key_events225[key_event_count225++]=event;
            continue;
        }
#endif
        client_handle_event(&event);
    }
}

#if RS2_SCENE225_RUNTIME
/* updateGame: release a previously started drag before CYCLELOGIC3. */
static void client_drag_input225(void) {
    if(comp_cfg.drag.active) {
        comp_cfg.drag.cycles++;
        if(abs(mouse_x-comp_cfg.drag.grab_x)>5 || abs(mouse_y-comp_cfg.drag.grab_y)>5)
            comp_cfg.drag.threshold=true;
    }
    if(mouse_button225==0 && (comp_cfg.drag.active || comp_cfg.scroll_drag.active)) {
        bool item_drag=comp_cfg.drag.active;
        ClientEvent release={.type=CLIENT_EVENT_MOUSE_UP,.button=CLIENT_BUTTON_LEFT,.x=mouse_x,.y=mouse_y};
        client_handle_event(&release);
        if(item_drag)cycle_click_button=0;
    }
}

/* handleMouseInput follows packet reads, actor updates and CYCLELOGIC3. */
static void client_mouse_input225(void) {
    if(cycle_click_button)client_handle_event(&mouse_press225);
    if(menu.visible && cycle_click_button!=1 && !menu_opened_cycle225 &&
       (mouse_x<menu.x-10 || mouse_x>menu.x+menu.width+10 ||
        mouse_y<menu.y-10 || mouse_y>menu.y+menu.height+10))menu.visible=false;
    menu_opened_cycle225=false;
}
#endif

// One client cycle: updateTitle, or updateGame with the packet reads (client.java:7665).
static const char *client_maps_dir(void) {
    const char *override=getenv("RS2_MAP_CACHE_DIR");if(override && *override)return override;
    FILE *probe = fopen("../engine/data/pack/client/maps/m50_50", "rb");
    if (probe) { fclose(probe); return "../engine/data/pack/client/maps"; }
    return "engine/data/pack/client/maps";
}

// client.buildScene for the announced region. Keep loading and retain the scene
// when allocation or compressed-map validation fails.
static bool client_build_scene(void) {
    GroundItem **saved_items=ground_items_detach(world3d);
    if(!saved_items) {fprintf(stderr,"Cannot retain ground items during rebuild\n");return false;}
    bool rebuilt=map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, &loc_cfg, client_maps_dir());
    loc_clear_model_cache(false);
    ground_items_restore(world3d,saved_items,rebuilt?streamer.base_tile_x-build_old_base_x:0,rebuilt?streamer.base_tile_z-build_old_base_z:0);
    if (!rebuilt) { fprintf(stderr, "Live map rebuild rejected; retaining loading state\n"); return false; }
    effects_clear(&effects);zone_sync_reset(&zones);
    // client.buildScene sends NO_TIMEOUT four times while building (client.java:8961-8990).
    for (int i = 0; i < 4; i++) if (!protocol_send_keepalive(&session)) reconnecting = true;
    if (has_minimap) client_build_minimap(sync_ctx.current_plane);
    printf("[VERIFY] Live REBUILD_NORMAL: %d mapsquares, base=(%d,%d)\n",
        streamer.region_count, streamer.base_tile_x, streamer.base_tile_z);
#if !RS2_SCENE225_RUNTIME
    cam.orbit_x = player.base.x;
    cam.orbit_z = player.base.z;
#endif
    return true;
}

static void client_login(bool reconnect);
static void client_draw_reconnect_overlay(void);
static void client_try_reconnect(void) {
    if (idle_timeout > 0) { client_logout(); return; }
    /* Original presents the retained viewport before blocking inside updateGame. */
    pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
    if (has_fonts) client_draw_reconnect_overlay();
    long long hash = (long long)client_frame_hash();
    if (session_log_mode() == SESSION_LOG_RECORD) {
        session_log_begin("P"); session_log_int(hash); session_log_end();
    } else if (session_log_mode() == SESSION_LOG_REPLAY) {
        long long expected=0;
        if (!session_log_take("P") || !session_log_take_int(&expected) || expected!=hash)
            session_log_desync("reconnect intermediate presentation differs");
    }
    if (plat.present) plat.present(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
    if (has_minimap) minimap.flag_scene_tile_x=0;
    ClientStream old_stream=session.stream;
    memset(&session.stream,0,sizeof(session.stream));
    session.stream.sock=INVALID_SOCKET_VAL;
    ingame=false;live_server=false;actions.live=false;
    client_login(true);
    /* The original keeps the previous socket until login has completed. */
    client_stream_close(&old_stream);
}

bool client_update(void) {
    client_login_dns_poll225();
    game_ticks++;
    if (!ingame) {
        if (!login_pending && title_screen_update(&title, title_click_button, title_click_x, title_click_y) == TITLE_LOGIN_REQUESTED) {
            // client.login(): show "Connecting to server..." before the blocking connect.
            title.message0[0] = '\0';
            snprintf(title.message1, sizeof(title.message1), "Connecting to server...");
            login_pending = true;
        }
        title_click_button = 0;
        return true;
    }
    title_click_button = 0;
    // Legacy input path. The revision225 path checks at handleMouseInput below.
#if !RS2_SCENE225_RUNTIME
    if (menu.visible && cycle_click_button != 1 &&
        (mouse_x < menu.x - 10 || mouse_x > menu.x + menu.width + 10 ||
         mouse_y < menu.y - 10 || mouse_y > menu.y + menu.height + 10)) {
        menu.visible = false;
    }
#endif
    if (system_update_timer > 1) system_update_timer--;
    if (idle_timeout > 0) idle_timeout--;
    sync_ctx.cycle=game_ticks;effects.cycle=game_ticks;
#if !RS2_SCENE225_RUNTIME
    if (comp_cfg.drag.active) comp_cfg.drag.cycles++;
#endif
    if (comp_cfg.scroll_drag.active && comp_cfg.scroll_drag.target && comp_cfg.scroll_drag.arrow_dir != 0) {
        comp_cfg.scroll_drag.arrow_hold_ticks++;
        if (comp_cfg.scroll_drag.arrow_hold_ticks >= 10 && (comp_cfg.scroll_drag.arrow_hold_ticks % 2 == 0)) {
            Component *target = comp_cfg.scroll_drag.target;
            target->scroll_position += comp_cfg.scroll_drag.arrow_dir * 16;
            int max_s = comp_cfg.scroll_drag.s_max - comp_cfg.scroll_drag.h;
            if (target->scroll_position > max_s) target->scroll_position = max_s;
            if (target->scroll_position < 0) target->scroll_position = 0;
        }
    }
    // A. Poll incoming network packets from live server (client.java:7665)
    if (live_server) {
        int opcode = 0;
        uint8_t payload[65536];
        size_t pkt_len = 0;
        int receive_status = 0;
        // client.updateGame: at most five packets per cycle (client.java:7665).
        for (int packets_read = 0; packets_read < 5 &&
             (receive_status=protocol_poll_packet(&session, &opcode, payload, sizeof(payload), &pkt_len)) > 0; packets_read++) {
            int zone_result=zone_sync_apply(&zones,world,world3d,&loc_cfg,collision_maps,sync_ctx.current_plane,opcode,payload,pkt_len);
            if(zone_result<0) {fprintf(stderr,"Malformed zone packet %d\n",opcode);logout_now=true;break;}
            if(zone_result>0) {
#if !RS2_SCENE225_RUNTIME
                if(zones.dirty && has_minimap) client_build_minimap(sync_ctx.current_plane);
#endif
                zones.dirty=false;continue;
            }
            int panel_update=interface_state_apply(&interfaces,opcode,payload,pkt_len);
            if(panel_update<0) {
                fprintf(stderr,"Malformed interface packet %d\n",opcode);
                logout_now=true;break;
            }
            if(panel_update>0) {
                comp_cfg.pressed_continue=false;
#if RS2_SCENE225_RUNTIME
                client_interface_reset225(component_get(&comp_cfg,interfaces.chat));
                client_interface_reset225(component_get(&comp_cfg,interfaces.viewport));
                client_interface_reset225(component_get(&comp_cfg,interfaces.sidebar));
#endif
                chatbox_close_count_dialog(&chatbox);
                comp_cfg.drag.active=false;
#if !RS2_SCENE225_RUNTIME
                menu.visible=false;
#endif
                if(trace) printf("[STATE] panels main=%d side=%d chat=%d\n",interfaces.viewport,interfaces.sidebar,interfaces.chat);
                continue;
            }
#if RS2_PIX3D_REV225_TEXTURE
            /* Original RESET_CLIENT_VARCACHE restores server values, updating
             * brightness only where the predicted local value differs. */
            if(opcode==193 && pkt_len==0)for(int id=0;id<MAX_VARPS;id++)
                if(varp_get(&varp_ctx,id)!=varp_ctx.server_varps[id] &&
                   varp_type_clientcode(&texture_varp_types,id)==1)
                    texture_varp_brightness(varp_ctx.server_varps[id]);
            int prior_varp=0,varp_id=-1;
            if((opcode==150||opcode==175)&&pkt_len>=2) {
                varp_id=(payload[0]<<8)|payload[1];
                prior_varp=varp_get(&varp_ctx,varp_id);
            }
#endif
            int update=component_apply_packet(&comp_cfg,&varp_ctx,&arena_perm,opcode,payload,pkt_len);
            if(update<0) {
                fprintf(stderr,"[Protocol] Malformed component packet %d (%zu bytes)\n",opcode,pkt_len);
                logout_now=true;break;
            }
            if(update>0) {
#if RS2_PIX3D_REV225_TEXTURE
                if(varp_id>=0 && varp_type_clientcode(&texture_varp_types,varp_id)==1 &&
                    varp_get(&varp_ctx,varp_id)!=prior_varp)
                    texture_varp_brightness(varp_get(&varp_ctx,varp_id));
#endif
                if(trace && (opcode==150 || opcode==175)) {
                    int id=(payload[0]<<8)|payload[1];printf("[STATE] varp=%d value=%d\n",id,varp_get(&varp_ctx,id));
                }
                if(trace && opcode==44 && pkt_len==6) {
                    int skill=payload[0];
                    printf("[STATE] stat=%d level=%d xp_times10=%d\n",skill,varp_ctx.skill_levels[skill],varp_ctx.skill_exp[skill]);
                }
                if(trace && (opcode==98 || opcode==213 || opcode==15)) {
                    int id=(payload[0]<<8)|payload[1];Component *inv=component_get(&comp_cfg,id);
                    printf("[STATE] inventory=%d",id);
                    if(inv && inv->inv_slot_obj_id) for(int slot=0;slot<inv->width*inv->height;slot++)
                        if(inv->inv_slot_obj_id[slot]) printf(" slot%d=%dx%d",slot,inv->inv_slot_obj_id[slot]-1,inv->inv_slot_obj_count[slot]);
                    printf("\n");
                }
                continue;
            }
            if(opcode==3 || opcode==13 || opcode==74 || opcode==239){
                if(!camera_packet225(&cam,opcode,payload,pkt_len,sync_ctx.current_plane,world3d->heightmap,world->tile_flags))fprintf(stderr,"[CAMERA] Invalid camera packet%d\n",opcode);
                continue;
            }
            if(opcode==136 || opcode==254){
                if(entity_sync_control225(&sync_ctx,opcode,payload,pkt_len)<0)fprintf(stderr,"[STATE] Invalid reset/multiway packet%d\n",opcode);
                continue;
            }
            if(opcode==133 || opcode==226){
                int result=protocol_tracking_control(&session,&input_tracking,opcode,payload,pkt_len,opcode==226?client_tracking_time():0);
                if(result==-1)fprintf(stderr,"[TRACK] Invalid control packet%d\n",opcode);
                if(result==-2)reconnecting=true;
                continue;
            }
            if(opcode==20 || opcode==80 || opcode==132 || opcode==220){
                int result=map_streamer_receive(&streamer,opcode,payload,pkt_len,client_maps_dir());
                if(result<0)fprintf(stderr,"[MAP] Invalid download packet%d\n",opcode);
                if(result==2)fprintf(stderr,"[MAP] Cache write failed; downloaded data retained in memory\n");
                if((opcode==20 || opcode==80) && result>0 && streamer.scene_state==1){if(!scene_build_pending){build_old_base_x=streamer.base_tile_x;build_old_base_z=streamer.base_tile_z;}scene_build_pending=true;}
                continue;
            }
            if (opcode == 4) { // MESSAGE_GAME
                bool blocked=false,added=false;
#if RS2_SCENE225_RUNTIME
                blocked=private_override_chat225;
#endif
                if(!chatbox_add_game_message225(&chatbox,payload,pkt_len,blocked,&added))
                    fprintf(stderr,"[CHAT] Invalid game-message packet or history allocation failure\n");
#if RS2_SCENE225_RUNTIME
                if(added && chatbox.type[0]==0 && sticky_chat_interface_id!=-1) {
                    snprintf(modal_message225,sizeof(modal_message225),"%s",chatbox.text[0]);
                    cycle_click_button=0;world_action_pending225=false;minimap_click_pending225=false;
                }
#endif
                if (added && trace)
                    printf("[STATE] game_message=%s\n",chatbox.text[0]);
            } else if(opcode==21) {
                if(!chatbox_apply_ignores(&chatbox,payload,pkt_len))
                    fprintf(stderr,"[CHAT] Invalid ignore-list packet\n");
            } else if(opcode==41) {
                bool blocked=false;
#if RS2_SCENE225_RUNTIME
                blocked=private_override_chat225;
#endif
                if(!chatbox_add_private_message(&chatbox,&chat_filter,payload,pkt_len,blocked))
                    fprintf(stderr,"[CHAT] Private message decode/filter failed\n");
            } else if(opcode==140){
                int result=last_login_receive225(&last_login225,&interfaces,&chatbox,&comp_cfg,&session,payload,pkt_len);
                if(result==-1)fprintf(stderr,"[LOGIN] Invalid last-login information\n");
                if(result==-2)reconnecting=true;
                client_login_dns_poll225();
            } else if (opcode == 12) { // SYNTH_SOUND
                if (pkt_len >= 5) {
                    int sound_id = (payload[0] << 8) | payload[1];
                    int loops = payload[2];
                    int delay = (payload[3] << 8) | payload[4];
                    if(varp_ctx.wave_enabled && !varp_ctx.music.low_memory)audio_queue_sound(sound_id,loops,delay);
                }
            } else if (opcode == 84) { // IF_SETTAB_ACTIVE
                if (pkt_len >= 1) {
                    int tab = payload[0];
                    if (tab >= 0 && tab < 14) selected_tab = tab;
                }
            } else if (opcode == 43) { // UPDATE_REBOOT_TIMER, client.read: g2 * 30
                system_update_timer = ((payload[0] << 8) | payload[1]) * 30;
            } else if (opcode == 142) { // LOGOUT: client.logout(), remaining input is discarded
                if(trace) printf("[STATE] server logout packet\n");
                logout_now=true;break;
            } else if (opcode == 146) { // IF_SETANIM
                if (pkt_len >= 4) {
                    int com_id = (payload[0] << 8) | payload[1];
                    int seq_id = (payload[2] << 8) | payload[3];
                    Component *c = component_get(&comp_cfg, com_id);
                    if (c) c->anim_id = seq_id;
                }
            } else if (opcode == 185) { // TUT_OPEN (sticky tutorial chatbox)
                if (pkt_len >= 2) {
                    int id = (payload[0] << 8) | payload[1];
                    sticky_chat_interface_id = (id == 65535) ? -1 : id;
                }
            } else if (opcode == 126) { // TUT_FLASH (tab flashing)
                if (pkt_len >= 1) {
                    flashing_tab = payload[0];
                    if (flashing_tab == selected_tab) {
                        selected_tab = (flashing_tab == 3) ? 1 : 3;
                    }
                }
            } else if (opcode == 25) { // HINT_ARROW
                if (pkt_len >= 6) {
                    int t = (int8_t)payload[0];
                    if (t == 1) {
                        hint_type = 1;
                        hint_npc = (payload[1] << 8) | payload[2];
                    } else if (t >= 2 && t <= 6) {
                        if (t == 2) {
                            hint_offset_x = 64;
                            hint_offset_z = 64;
                        } else if (t == 3) {
                            hint_offset_x = 0;
                            hint_offset_z = 64;
                        } else if (t == 4) {
                            hint_offset_x = 128;
                            hint_offset_z = 64;
                        } else if (t == 5) {
                            hint_offset_x = 64;
                            hint_offset_z = 0;
                        } else if (t == 6) {
                            hint_offset_x = 64;
                            hint_offset_z = 128;
                        }
                        hint_type = 2;
                        hint_tile_x = (payload[1] << 8) | payload[2];
                        hint_tile_z = (payload[3] << 8) | payload[4];
                        hint_height = payload[5];
                    } else if (t == 10) {
                        hint_type = 10;
                        hint_player = (payload[1] << 8) | payload[2];
                    } else {
                        hint_type = 0;
                    }
                    if (trace) printf("[HINT] type=%d npc=%d tile=(%d,%d) h=%d pl=%d\n", hint_type, hint_npc, hint_tile_x, hint_tile_z, hint_height, hint_player);
                }
            } else if (opcode == 167) { // IF_SETTAB
                if (pkt_len >= 3) {
                    int com_id = (payload[0] << 8) | payload[1];
                    int tab = payload[2];
                    if (com_id == 65535) com_id = -1;
                    if (tab >= 0 && tab < 14) tab_interface_id[tab] = com_id;
                }
            } else if (opcode == 204) { // IF_SETNPCHEAD
                if (pkt_len >= 4) {
                    int com_id = (payload[0] << 8) | payload[1];
                    int npc_id = (payload[2] << 8) | payload[3];
                    Component *c = component_get(&comp_cfg, com_id);
                    if (c) {
                        NpcType npc_type;
                        if (npc_get(&npc_cfg, npc_id, &npc_type)) {
                            c->model_id = npc_id;
                            c->model = npc_get_head_model(&npc_type, &arena_perm);
                            npc_free_entry(&npc_type);
                        }
                    }
                }
            } else if (opcode == 197) { // IF_SETPLAYERHEAD
                if (pkt_len >= 2) {
                    int com_id = (payload[0] << 8) | payload[1];
                    Component *c = component_get(&comp_cfg, com_id);
                    if (c) {
                        c->model = player_entity_get_head_model(&player, &idk_cfg, &obj_cfg, &arena_perm);
                    }
                }
            } else if (opcode == 54) { // MIDI_SONG
                char song_name[64];
                size_t name_len = 0;
                // Revision225 gjstr/pjstr uses byte10, followed by two g4s.
                while (name_len < pkt_len && payload[name_len] != 10) name_len++;
                if (name_len >= sizeof(song_name) || name_len + 9 != pkt_len ||
                    memchr(payload, 0, name_len) != NULL) {
                    fprintf(stderr,"[Protocol] Malformed MIDI_SONG\n");
                    logout_now=true;break;
                }
                memcpy(song_name,payload,name_len);
                song_name[name_len]='\0';
                size_t pos=name_len+1;
                uint32_t crc=((uint32_t)payload[pos]<<24) | ((uint32_t)payload[pos+1]<<16) |
                    ((uint32_t)payload[pos+2]<<8) | payload[pos+3];
                uint32_t size=((uint32_t)payload[pos+4]<<24) | ((uint32_t)payload[pos+5]<<16) |
                    ((uint32_t)payload[pos+6]<<8) | payload[pos+7];
                (void)music_song225(&varp_ctx.music,song_name,crc,size);
            } else if (opcode == 212) { // MIDI_JINGLE
                if(varp_ctx.music.active && !varp_ctx.music.low_memory){
                if (pkt_len < 6) {
                    fprintf(stderr,"[Protocol] Malformed MIDI_JINGLE\n");
                    logout_now=true;break;
                }
                uint32_t uncomp_len = (uint32_t)payload[2]<<24 | (uint32_t)payload[3]<<16 |
                    (uint32_t)payload[4]<<8 | payload[5];
                if (uncomp_len > MIDI_MAX_DECODED ||
                    !midi_request_jingle225(payload+6,(int)(pkt_len-6),(int)uncomp_len))
                    fprintf(stderr,"[MIDI] Jingle command rejected or playback unavailable\n");
                (void)music_jingle225(&varp_ctx.music,(int)((uint16_t)payload[0]<<8 | payload[1]));
                }
            } else if (opcode == 243) { // P_COUNTDIALOG
                chatbox_open_count_dialog(&chatbox);
            } else if (opcode == 152) { // UPDATE_FRIEND: original standalone nodeId is10.
                if (!chatbox_apply_friend(&chatbox, payload, pkt_len, 10)) {
                    fprintf(stderr,"[CHAT] Malformed friend update\n");
                    session.in_game = false;
                }
            } else if (opcode == 32) { // CHAT_FILTER_SETTINGS
                if (pkt_len >= 3) {
                    public_chat_setting = payload[0];
                    private_chat_setting = payload[1];
                    trade_chat_setting = payload[2];
                }
            } else if (opcode == 139 && pkt_len==2) { // UPDATE_PID: original localPid, separate from local render slot 2047.
                zones.local_pid=(payload[0]<<8)|payload[1];
            } else if (opcode == 184) { // PLAYER_INFO
                int previous_plane=sync_ctx.current_plane;
                sync_ctx.player_chat=client_player_chat;sync_ctx.player_chat_user=NULL;
                Packet pinfo = packet_wrap(payload, pkt_len);
                if(!entity_sync_player_info(&sync_ctx,&pinfo,pkt_len)) {
                    fprintf(stderr,"[Protocol] Invalid PLAYER_INFO\n");logout_now=true;break;
                }
                bool rebuilt_scene225=false;
                if (scene_build_pending && streamer.scene_state==1) { // original read184 gate
                    if(client_build_scene()){scene_build_pending=false;rebuilt_scene225=true;}
                }
                if(previous_plane!=sync_ctx.current_plane && streamer.scene_state==2 && has_minimap && (!RS2_SCENE225_RUNTIME || !rebuilt_scene225)) {
                    client_build_minimap(sync_ctx.current_plane);
                    if(trace) printf("[STATE] plane=%d minimap rebuilt\n",sync_ctx.current_plane);
                }
#if !RS2_SCENE225_RUNTIME
                if(player.name[0]) snprintf(chatbox.username,sizeof(chatbox.username),"%.31s",player.name);
#endif
            } else if (opcode == 19) { // UNSET_MAP_FLAG: zero payload bytes
                if (has_minimap) minimap.flag_scene_tile_x = 0;
                if (trace) printf("[STATE] destination cleared by server\n");
            } else if (opcode == 1) { // NPC_INFO
                Packet ninfo = packet_wrap(payload, pkt_len);
                if(!entity_sync_npc_info(&sync_ctx,&ninfo,pkt_len)) {
                    fprintf(stderr,"[Protocol] Invalid NPC_INFO\n");logout_now=true;break;
                }
            } else if (opcode == 237) { // REBUILD_NORMAL
                // client.java:9912-9917: the same centre zone with a scene already begun is ignored.
                if (pkt_len >= 4 && streamer.scene_state != 0 &&
                    ((payload[0] << 8) | payload[1]) == streamer.center_zone_x &&
                    ((payload[2] << 8) | payload[3]) == streamer.center_zone_z) continue;
#if RS2_SCENE225_RUNTIME
                bool retain_scene225=streamer.scene_state==2;
#endif
                int old_base_x=streamer.base_tile_x,old_base_z=streamer.base_tile_z;
                Packet rebuild_pkt = packet_wrap(payload, pkt_len);
                if (map_streamer_parse_rebuild_packet(&streamer, &rebuild_pkt)) {
                    cam.cutscene=false;
                    entity_sync_rebase(&sync_ctx,streamer.base_tile_x-old_base_x,streamer.base_tile_z-old_base_z);
                    if (has_minimap && minimap.flag_scene_tile_x != 0) {
                        minimap.flag_scene_tile_x -= streamer.base_tile_x-old_base_x;
                        minimap.flag_scene_tile_z -= streamer.base_tile_z-old_base_z;
                        if(trace) printf("[NAV] rebased destination world=(%d,%d)\n",
                            streamer.base_tile_x+minimap.flag_scene_tile_x,streamer.base_tile_z+minimap.flag_scene_tile_z);
                    }
                    uint8_t missing[255];
                    int missing_length = map_streamer_prepare(&streamer, client_maps_dir(), missing, (int)sizeof(missing));
                    if(missing_length<0){fprintf(stderr,"[MAP] Could not prepare region cache state\n");logout_now=true;break;}
                    if (!protocol_send_map_request(&session, missing, missing_length)) reconnecting = true;   // IOException -> tryReconnect
                    if (missing_length) fprintf(stderr, "[Protocol] %d map files missing locally; requested from the server\n", missing_length / 3);
#if RS2_SCENE225_RUNTIME
                    client_loading_message225(missing_length>0,retain_scene225);
#endif
                    // sceneState = 1: the scene is built on the next PLAYER_INFO (client.java:10801-10806).
                    if (!scene_build_pending) { build_old_base_x = old_base_x; build_old_base_z = old_base_z; }
                    scene_build_pending = true;
                }
            }
        }
        if(receive_status<0 && !logout_now) {
            // client.read(): IOException -> tryReconnect, other failures -> logout.
            if(session.protocol_error) logout_now=true; else reconnecting=true;
        }
        /* read() recovers before updateGame increments the recovered idle counter. */
        if (reconnecting && !logout_now) client_try_reconnect();
        if (!ingame) return true;
        if(!logout_now){audio_tick225(client_tracking_time);music_tick225(&varp_ctx.music);midi_apply_music225(&varp_ctx.music);}
        if(!logout_now && !protocol_flush_tracking(&session,&input_tracking))reconnecting=true;
        if (!logout_now && protocol_tick_network_idle(&session)) client_try_reconnect();
        if (!ingame) return true;
        if(logout_now) {logout_now=false;client_logout();return true;}
    }
    if(!live_server){audio_tick225(client_tracking_time);music_tick225(&varp_ctx.music);midi_apply_music225(&varp_ctx.music);}
#if !RS2_SCENE225_RUNTIME
    int click_button = cycle_click_button;
    cycle_click_button = 0;
#endif

    if (live_server) {
        // client.updatePlayers (CYCLELOGIC6), updateMergeLocs (CYCLELOGIC5 once the scene is
        // built), the camera position while an arrow is held, then CYCLELOGIC3, and
        // handleTabInput's CYCLELOGIC1 on a left click (client.java:7731-7826).
        if (++cyclelogic6 > 1406) { cyclelogic6 = 0; protocol_send_cyclelogic6(&session); }
        if (streamer.scene_state == 2 && ++cyclelogic5 > 85) { cyclelogic5 = 0; protocol_send_cyclelogic5(&session); }
        if ((held.left || held.right || held.up || held.down) && camera_moved_write++ > 5) {
            camera_moved_write = 0;
            protocol_send_camera_position(&session, cam.orbit_pitch, cam.orbit_yaw, minimap_anticheat_angle, minimap_zoom);
        }
#if !RS2_SCENE225_RUNTIME
        if (++cyclelogic3 > 127) { cyclelogic3 = 0; protocol_send_cyclelogic3(&session); }
        if (click_button == 1 && ++cyclelogic1 > 150) { cyclelogic1 = 0; protocol_send_cyclelogic1(&session); }
#endif
    }

    // B. Camera & Player Movement Controls (client.java:6671-6684)
    bool key_left = held.left;
    bool key_right = held.right;
    bool key_up = held.up;
    bool key_down = held.down;

#if !RS2_SCENE225_RUNTIME
    camera_update(&cam, player.base.x, player.base.z, sync_ctx.current_plane, world3d->heightmap, world->tile_flags,
                  key_left, key_right, key_up, key_down);
#endif
    int cur_gy = camera_get_height(sync_ctx.current_plane, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
    player.base.y = cur_gy;

    // Player keyboard movement controls (relative to camera yaw)
    int forward = 0;
    int strafe = 0;
    if (held.w) forward += 1;
    if (held.s) forward -= 1;
    if (held.a) strafe -= 1;
    if (held.d) strafe += 1;

    if (offline && (forward != 0 || strafe != 0) && player.base.path_length == 0) {
        int sin_y = pix3d_sin_table[cam.render_yaw & 2047];
        int cos_y = pix3d_cos_table[cam.render_yaw & 2047];
        int dx = (strafe * cos_y + forward * sin_y) / 256;
        int dz = (forward * cos_y - strafe * sin_y) / 256;
        int target_tx = (player.base.x + dx) / 128;
        int target_tz = (player.base.z + dz) / 128;
        if (target_tx >= 1 && target_tx < 103 && target_tz >= 1 && target_tz < 103) {
            PathResult res;
            if (pathfinder_find_path(&pathfinder, &collision_maps[0],
                                     player.base.path_tile_x[0], player.base.path_tile_z[0],
                                     target_tx, target_tz, false, &res)) {
                pathing_entity_queue_path(&player.base, res.steps_x, res.steps_z, res.step_count, false);
                if(live_server) {
                    if(!protocol_send_route(&session,streamer.base_tile_x,streamer.base_tile_z,player.base.path_tile_x[0],player.base.path_tile_z[0],
                        res.steps_x,res.steps_z,res.step_count,false)) fprintf(stderr,"[ACTION] Route send failed\n");
                }
            }
        }
    }

    // C. Entity Logic Updates (authentic 50 Hz stride & frame progression)
#if RS2_SCENE225_RUNTIME
    client_entity_movement(&player.base);
#else
    if(!pathing_entity_update_exact(&player.base,&seq_cfg,game_ticks))client_entity_movement(&player.base);
#endif
    if (has_minimap && player.base.x / 128 == minimap.flag_scene_tile_x &&
        player.base.z / 128 == minimap.flag_scene_tile_z) {
        if(trace && minimap.flag_scene_tile_x) printf("[NAV] destination reached world=(%d,%d)\n",
            streamer.base_tile_x+minimap.flag_scene_tile_x,streamer.base_tile_z+minimap.flag_scene_tile_z);
        minimap.flag_scene_tile_x = 0;
    }
    entity_sync_update_facing(&sync_ctx,&player.base,streamer.base_tile_x,streamer.base_tile_z,zones.local_pid);
    pathing_entity_update_sequences(&player.base, &seq_cfg);

    if(live_server) {
        for(int i=0;i<sync_ctx.npc_count;i++) {
            NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];if(!npc) continue;
            client_entity_movement(&npc->base);entity_sync_update_facing(&sync_ctx,&npc->base,streamer.base_tile_x,streamer.base_tile_z,zones.local_pid);pathing_entity_update_sequences(&npc->base,&seq_cfg);
        }
        for(int i=0;i<sync_ctx.player_count;i++) {
            PlayerEntity *other=sync_ctx.players[sync_ctx.player_indices[i]];if(!other) continue;
#if RS2_SCENE225_RUNTIME
            client_entity_movement(&other->base);
#else
            if(!pathing_entity_update_exact(&other->base,&seq_cfg,game_ticks))client_entity_movement(&other->base);
#endif
            entity_sync_update_facing(&sync_ctx,&other->base,streamer.base_tile_x,streamer.base_tile_z,zones.local_pid);pathing_entity_update_sequences(&other->base,&seq_cfg);
        }
    }
#if RS2_SCENE225_RUNTIME
    entity_sync_tick_chats(&sync_ctx);
#endif
    client_entity_movement(&hans.base);
    pathing_entity_update_facing(&hans.base);
    pathing_entity_update_sequences(&hans.base, &seq_cfg);



    // D. Cross animation progression (client.java:7744-7749)
    click_feedback_tick(&click_feedback);
#if RS2_SCENE225_RUNTIME
    client_drag_input225();
    if(live_server && ++cyclelogic3>127) {cyclelogic3=0;protocol_send_cyclelogic3(&session);}
    client_walk_result225();
    client_mouse_input225();
    if(world_action_pending225) {
        world_action_pending225=false;
        actions.click_x=world_action_x225;actions.click_y=world_action_y225;
        ActionResult result=client_action_dispatch(&actions,world_action225);
        if(result==ACTION_UNAVAILABLE)fprintf(stderr,"[ACTION] Unsupported action type %d; no packet sent\n",world_action225.type);
    }
#endif
#if RS2_SCENE225_RUNTIME
    if(minimap_click_pending225) {
        minimap_click_pending225=false;
        client_minimap_route225(minimap_click_x225,minimap_click_y225);
    }
    if(live_server && cycle_click_button==1 && ++cyclelogic1>150) {
        cyclelogic1=0;protocol_send_cyclelogic1(&session);
    }
    cycle_click_button=0;
#endif
#if RS2_SCENE225_RUNTIME
    /* updateGame advances actors before updateOrbitCamera, only in sceneState 2. */
    if(!live_server || streamer.scene_state==2)camera_update(&cam, player.base.x, player.base.z, sync_ctx.current_plane, world3d->heightmap, world->tile_flags,
                  key_left, key_right, key_up, key_down);
#endif
    if(!live_server || streamer.scene_state==2)camera_cutscene_tick225(&cam,sync_ctx.current_plane,world3d->heightmap,world->tile_flags);
    camera_modifier_tick225(&cam);
    actor_spotanim_tick(&player.base,&spot_cfg,game_ticks);
    for(int i=0;i<sync_ctx.npc_count;i++)if(sync_ctx.npcs[sync_ctx.npc_indices[i]])actor_spotanim_tick(&sync_ctx.npcs[sync_ctx.npc_indices[i]]->base,&spot_cfg,game_ticks);
    for(int i=0;i<sync_ctx.player_count;i++)if(sync_ctx.players[sync_ctx.player_indices[i]])actor_spotanim_tick(&sync_ctx.players[sync_ctx.player_indices[i]]->base,&spot_cfg,game_ticks);
    effects_tick(&effects,world,world3d,&sync_ctx,game_ticks);
    if(!location_merge_tick(&zones,world,world3d,&loc_cfg,collision_maps,game_ticks)) {fprintf(stderr,"Cannot update merged scenery\n");running=false;return false;}
#if !RS2_SCENE225_RUNTIME
    if(zones.dirty && has_minimap) client_build_minimap(sync_ctx.current_plane);
#endif
    zones.dirty=false;

#if RS2_SCENE225_RUNTIME
    /* Original handleInputKey runs after world input/camera and before idle drift. */
    for(int i=0;i<key_event_count225;i++)client_handle_event(&key_events225[i]);
    key_event_count225=0;
#endif

    // client.java:7902-7917, the end of updateGame: ANTICHEAT_CYCLELOGIC4 every 111
    // cycles, NO_TIMEOUT after 50 cycles without output, and heartbeatTimer reset
    // whenever the cycle sent anything.
    if (live_server && !reconnecting) {
        // client.java:7841-7900: IDLE_TIMER after 4500 idle cycles, then the anticheat drift.
        if (++idle_cycles > 4500) { idle_timeout = 250; idle_cycles -= 500; protocol_send_idle(&session); }
        if (++camera_offset_cycle > 500) {
            camera_offset_cycle = 0;
            int bits = (int)(math_random() * 8.0);
            if ((bits & 1) == 1) cam.anticheat_x += camera_offset_x_modifier;
            if ((bits & 2) == 2) cam.anticheat_z += camera_offset_z_modifier;
            if ((bits & 4) == 4) cam.anticheat_yaw += camera_offset_yaw_modifier;
        }
        if (cam.anticheat_x < -50) camera_offset_x_modifier = 2;
        if (cam.anticheat_x > 50) camera_offset_x_modifier = -2;
        if (cam.anticheat_z < -55) camera_offset_z_modifier = 2;
        if (cam.anticheat_z > 55) camera_offset_z_modifier = -2;
        if (cam.anticheat_yaw < -40) camera_offset_yaw_modifier = 1;
        if (cam.anticheat_yaw > 40) camera_offset_yaw_modifier = -1;
        if (++minimap_offset_cycle > 500) {
            minimap_offset_cycle = 0;
            int bits = (int)(math_random() * 8.0);
            if ((bits & 1) == 1) minimap_anticheat_angle += minimap_angle_modifier;
            if ((bits & 2) == 2) minimap_zoom += minimap_zoom_modifier;
        }
        if (minimap_anticheat_angle < -60) minimap_angle_modifier = 2;
        if (minimap_anticheat_angle > 60) minimap_angle_modifier = -2;
        if (minimap_zoom < -20) minimap_zoom_modifier = 1;
        if (minimap_zoom > 10) minimap_zoom_modifier = -1;

        if (++cyclelogic4 > 110) { cyclelogic4 = 0; protocol_send_cyclelogic4(&session); }
        if (++heartbeat_timer > 50) protocol_send_keepalive(&session);
        // Write the cycle's packets; an overflowed or failed write is an IOException.
        if (session.stream.out_overflow) reconnecting = true;
        if (client_stream_pending(&session.stream)) {
            if (!client_stream_flush(&session.stream)) reconnecting = true;
            heartbeat_timer = 0;
        }
    }

    if (reconnecting) client_try_reconnect();
    if (getenv("RS2_TRACE_RECOVERY")) printf("[RECOVERY CLOCK] frame=%d offsets=%d,%d state=%llu map=%d,%d\n",rendered_frames,camera_offset_cycle,minimap_offset_cycle,(unsigned long long)math_random_state(),minimap_anticheat_angle,minimap_zoom);
    return true;
}

#if RS2_SCENE225_RUNTIME
static void scene225_pick(uint32_t bitset){
    if(picker.picked_count<MAX_PICKED)picker.picked_bitsets[picker.picked_count++]=bitset;
}
static bool scene225_cursor(void *context,const SceneCall225 *call){
    if(call->kind!=SCENE225_MODEL || call->bitset<=0)return true;
    SceneFrame225 *frame=context;uint32_t bits=(uint32_t)call->bitset;int type=(bits>>29)&3,id=(bits>>14)&32767;
    bool loc=type==2 && id==verification_loc_id,obj=false,npc=false;
    if(type==3 && verification_obj_id>=0){
        for(GroundItem *item=world3d->tiles[frame->level][bits&127][(bits>>7)&127].items;item;item=item->next)
            if(item->id==verification_obj_id)obj=true;
    }
    if(type==1 && verification_npc_type>=0){
        NpcEntity *n=offline && id==0?&hans:live_server && id<MAX_NPCS?sync_ctx.npcs[id]:NULL;
        npc=n && n->has_type && n->type.index==verification_npc_type;
    }
    if(!loc && !obj && !npc)return true;
    const SceneModel225 *model=world3d_scene_resolve225(scene_runtime225_build(scene_runtime),call->model);
    if(!model)return true;
    int sx,sy;
    if(!project_from_point(call->dx+frame->eye_x,call->dy+frame->eye_y-model->bounds.max_y/2,call->dz+frame->eye_z,
        frame->eye_x,frame->eye_y,frame->eye_z,frame->pitch,frame->yaw,&sx,&sy))return true;
    int best=100000,bx=-1,by=-1;
    for(int dy=-36;dy<=36;dy+=2)for(int dx=-36;dx<=36;dx+=2){
        int x=sx+dx,y=sy+dy,score=dx*dx+dy*dy;if(score>=best || x<0 || x>=VIEWPORT_W || y<0 || y>=VIEWPORT_H)continue;
        ModelPick225 pick={true,model->pickable,x,y,call->bitset,false};
        if(model_probe_world_bounds225(model->model,&model->bounds,call->yaw,frame->pitch,frame->yaw,call->dx,call->dy,call->dz,&pick) && pick.picked){best=score;bx=x;by=y;}
    }
    if(bx>=0){
        verification_cursor_x=bx+VIEWPORT_X;verification_cursor_y=by+VIEWPORT_Y;
        ClientEvent event={0};event.type=CLIENT_EVENT_MOUSE_MOVE;event.x=verification_cursor_x;event.y=verification_cursor_y;client_queue_event(&event);
        printf("[INPUT] Scene225 rendered type=%d id=%d pointer=(%d,%d)\n",type,id,event.x,event.y);
        if(loc)verification_loc_id=-1;
        if(obj)verification_obj_id=-1;
        if(npc)verification_npc_type=-1;
    }
    return true;
}
static bool client_scene225(int eye_x,int eye_y,int eye_z,int pitch,int yaw){
    if(!scene_runtime)scene_runtime=scene_runtime225_create(world,world3d,&loc_cfg,&seq_cfg,&idk_cfg,&obj_cfg,&spot_cfg);
    if(!scene_runtime){fprintf(stderr,"[Scene225] Cannot construct runtime scene\n");return false;}
    arena_reset(&arena_frame);memset(scene_pixels,0,sizeof(scene_pixels));
    pix2d_bind(scene_pixels,VIEWPORT_W,VIEWPORT_H);pix3d_init_2d();pix3d_set_trans(0);pix3d_set_hclip(false);
    SceneFrame225 frame={0};frame.local=&player;frame.entities=live_server?&sync_ctx:NULL;
    frame.offline_npc=offline?&hans:NULL;frame.effects=&effects;frame.cycle=game_ticks;frame.level=sync_ctx.current_plane;
    frame.eye_x=eye_x;frame.eye_y=eye_y;frame.eye_z=eye_z;frame.pitch=pitch;frame.yaw=yaw;
    frame.top_level=cam.cutscene?camera_cutscene_top225(&cam,sync_ctx.current_plane,world3d->heightmap,world->tile_flags):scene_runtime225_top_level(world,sync_ctx.current_plane,cam.eye_x,cam.eye_z,player.base.x,player.base.z,cam.render_pitch);frame.check_hover=picker.check_hover;frame.mouse_x=mouse_x-VIEWPORT_X;frame.mouse_y=mouse_y-VIEWPORT_Y;
    frame.taking_input=world3d->taking_input;frame.ground_mouse_x=world3d->mouse_x-VIEWPORT_X;frame.ground_mouse_y=world3d->mouse_y-VIEWPORT_Y;
    frame.click_x=world3d->click_tile_x;frame.click_z=world3d->click_tile_z;
    clock_t begin=clock();bool ok=scene_runtime225_frame(scene_runtime,&frame,scene225_cursor,&frame);
    const SceneRaster225 *result=scene_runtime225_result(scene_runtime);
    if(!ok){fprintf(stderr,"[Scene225] Draw failed error=%d kind=%d\n",result->error,result->failed_call.kind);return false;}
    world3d->taking_input=result->view.taking_input;world3d->click_tile_x=result->view.click_tile_x;world3d->click_tile_z=result->view.click_tile_z;
    picker.picked_count=0;
    for(size_t i=0;i<result->picking.count;i++){
        /* client.buildViewportMenu skips only an adjacent repeated bitset. */
        if(i && result->picking.bitsets[i]==result->picking.bitsets[i-1])continue;
        uint32_t bits=(uint32_t)result->picking.bitsets[i];int type=(bits>>29)&3,id=(bits>>14)&32767;
        int x=(int)(bits&127),z=(int)((bits>>7)&127);
        if(type==3){
            for(GroundItem *item=world3d->tiles[frame.level][x][z].items;item;item=item->next)
                scene225_pick(mouse_picker_make_bitset(3,item->id,x,z));
        }else if(type==1){
            NpcEntity *n=offline && id==0?&hans:live_server && id<MAX_NPCS?sync_ctx.npcs[id]:NULL;
            if(n)scene225_pick(mouse_picker_make_bitset(1,id,n->base.x>>7,n->base.z>>7));
        }else if(type==0){
            PlayerEntity *p=id==2047?&player:live_server && id<MAX_PLAYERS?sync_ctx.players[id]:NULL;
            if(p)scene225_pick(mouse_picker_make_bitset(0,id,p->base.x>>7,p->base.z>>7));
        }else scene225_pick(bits);
    }
    if(trace && (capture_this_frame || rendered_frames==0))printf("[Scene225] models=%llu terrain=%llu hits=%zu cpu_ms=%llu revision=%llu\n",
        (unsigned long long)result->models,(unsigned long long)(result->underlays+result->overlays),result->picking.count,
        (unsigned long long)((clock()-begin)*1000/CLOCKS_PER_SEC),(unsigned long long)world3d->structure_revision);
    if(trace && capture_this_frame)printf("[ANIM] Scene225 local transform=%d\n",current_entity_frame(&player.base,&seq_cfg));
    if(trace && capture_this_frame)printf("[Scene225 STATE] frame=%d local=(%d,%d) path=(%d,%d) map=(%d,%d) menu=%d hue=%d light=%d low_detail=%d\n",
        rendered_frames,player.base.x,player.base.z,player.base.path_tile_x[0],player.base.path_tile_z[0],
        minimap_anticheat_angle,minimap_zoom,menu.visible,world->random_hue_offset,world->random_lightness_offset,pix3d_low_detail);
    for(int y=0;y<VIEWPORT_H;y++)memcpy(pixels+(VIEWPORT_Y+y)*CLIENT_WIDTH+VIEWPORT_X,scene_pixels+y*VIEWPORT_W,VIEWPORT_W*sizeof(uint32_t));
    pix2d_bind(pixels,CLIENT_WIDTH,CLIENT_HEIGHT);pix3d_init_2d();
    pix2d_set_clip(VIEWPORT_X,VIEWPORT_Y,VIEWPORT_X+VIEWPORT_W,VIEWPORT_Y+VIEWPORT_H);
    pix3d_center_x=VIEWPORT_X+VIEWPORT_W/2;pix3d_center_y=VIEWPORT_Y+VIEWPORT_H/2;return true;
}
#endif
#if RS2_SCENE225_RUNTIME
static InterfacePose225 *interface_pose_for225(Component *c) {
    if(!interface_pose225)interface_pose225=arena_alloc_zero(&arena_perm,(size_t)comp_cfg.count*sizeof(*interface_pose225));
    return interface_pose225 && c && c->id>=0 && c->id<comp_cfg.count?&interface_pose225[c->id]:NULL;
}
static void client_interface_reset225(Component *root) {
    if(!root)return;
    for(int i=0;i<root->child_count;i++) {
        Component *c=component_get(&comp_cfg,root->child_id[i]);if(!c)continue;
        InterfacePose225 *state=interface_pose_for225(c);if(state){state->frame=0;state->cycle=0;}
        if(c->type==COMPONENT_TYPE_MODEL_UI)client_interface_reset225(c);
    }
}
static void client_interface_pose225(Component *c,int elapsed,bool restore) {
    if(!c)return;
    if(c->type==COMPONENT_TYPE_MODEL_3D) {
        InterfacePose225 *state=interface_pose_for225(c);if(!state){running=false;return;}
        if(restore) {if(state->saved){c->model=state->saved;state->saved=NULL;}}
        else {
            int id=varp_eval_component_triggers(&varp_ctx,&comp_cfg,c)?c->active_anim_id:c->anim_id;
            SeqType *seq=id>=0 && id<seq_cfg.count?&seq_cfg.instances[id]:NULL;
            if(seq && seq->frame_count>0) {
                if(state->frame>=seq->frame_count)state->frame=0;
                state->cycle+=elapsed;
                while(state->cycle>seq->delay[state->frame]) {
                    state->cycle-=seq->delay[state->frame]+1;state->frame++;
                    if(state->frame>=seq->frame_count) {
                        state->frame-=seq->replayoff;
                        if(state->frame<0 || state->frame>=seq->frame_count)state->frame=0;
                    }
                }
            }
            bool transforms=seq && seq->frame_count>0 && (seq->frames[state->frame]!=-1 || seq->iframes[state->frame]!=-1);
            if(c->model && !(component_model_colourless225(&comp_cfg,c->model) && !transforms)) {
                Model *posed=model_copy(c->model,&arena_frame);
                if(!posed){running=false;return;}
                if(seq && seq->frame_count>0) {
                    model_create_label_references(posed,&arena_frame);
                    model_apply_transform(posed,seq->frames[state->frame]);
                    model_apply_transform(posed,seq->iframes[state->frame]);
                }
                model_calculate_normals(posed,64,768,-50,-10,-50);
                state->saved=c->model;c->model=posed;
            }
        }
    }
    for(int i=0;i<c->child_count;i++)client_interface_pose225(component_get(&comp_cfg,c->child_id[i]),elapsed,restore);
}
static void client_viewport_interface225(Component *root,int elapsed) {
    static uint32_t ui_pixels225[512*334];
    if(!root)return;
    uint32_t *saved=pix2d_pixels;int width=pix2d_width,height=pix2d_height;
    int left=pix2d_clip_left,top=pix2d_clip_top,right=pix2d_clip_right,bottom=pix2d_clip_bottom;
    int cx=pix3d_center_x,cy=pix3d_center_y;size_t mark=arena_frame.offset;
    for(int y=0;y<334;y++)memcpy(ui_pixels225+y*512,saved+(VIEWPORT_Y+y)*width+VIEWPORT_X,512*sizeof(uint32_t));
    client_interface_pose225(root,elapsed,false);pix2d_bind(ui_pixels225,512,334);pix3d_init_2d();
    component_draw_with_varps(&comp_cfg,root,0,0,0,&varp_ctx);
    client_interface_pose225(root,0,true);arena_frame.offset=mark;
    pix2d_bind(saved,width,height);pix3d_init_2d();pix2d_set_clip(left,top,right,bottom);pix3d_center_x=cx;pix3d_center_y=cy;
    for(int y=0;y<334;y++)memcpy(saved+(VIEWPORT_Y+y)*width+VIEWPORT_X,ui_pixels225+y*512,512*sizeof(uint32_t));
}
static void client_chat_interface225(Component *root,int elapsed) {
    if(!root)return;
    uint32_t *saved=pix2d_pixels;int width=pix2d_width,height=pix2d_height;
    int left=pix2d_clip_left,top=pix2d_clip_top,right=pix2d_clip_right,bottom=pix2d_clip_bottom;
    int cx=pix3d_center_x,cy=pix3d_center_y;size_t mark=arena_frame.offset;
    for(int y=0;y<96;y++)memcpy(chat_pixels225+y*479,saved+(375+y)*width+22,479*sizeof(uint32_t));
    client_interface_pose225(root,elapsed,false);
    pix2d_bind(chat_pixels225,479,96);pix3d_init_2d();
    component_draw_with_varps(&comp_cfg,root,0,0,0,&varp_ctx);
    client_interface_pose225(root,0,true);arena_frame.offset=mark;
    pix2d_bind(saved,width,height);pix3d_init_2d();pix2d_set_clip(left,top,right,bottom);
    pix3d_center_x=cx;pix3d_center_y=cy;
    for(int y=0;y<96;y++)memcpy(saved+(375+y)*width+22,chat_pixels225+y*479,479*sizeof(uint32_t));
}
#endif

static void client_draw_system_update(void) {
    if (!system_update_timer) return;
    int seconds = system_update_timer / 50;
    char message[64];
    snprintf(message, sizeof(message), "System update in: %d:%02d", seconds / 60, seconds % 60);
    pix2d_set_clip(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y + VIEWPORT_H);
    pixfont_draw_string(&font_p12, VIEWPORT_X + 4, VIEWPORT_Y + 329, 0xffff00, message);
    pix2d_reset_clip();
}

static void client_draw_reconnect_overlay(void) {
        // client.tryReconnect() message, drawn into areaViewport at (8,11).
        pix2d_set_clip(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y + VIEWPORT_H);
        pixfont_draw_string_center(&font_p12, VIEWPORT_X + 257, VIEWPORT_Y + 144, 0, "Connection lost");
        pixfont_draw_string_center(&font_p12, VIEWPORT_X + 256, VIEWPORT_Y + 143, 0xFFFFFF, "Connection lost");
        pixfont_draw_string_center(&font_p12, VIEWPORT_X + 257, VIEWPORT_Y + 159, 0, "Please wait - attempting to reestablish");
        pixfont_draw_string_center(&font_p12, VIEWPORT_X + 256, VIEWPORT_Y + 158, 0xFFFFFF, "Please wait - attempting to reestablish");
        pix2d_reset_clip();
}

#if RS2_SCENE225_RUNTIME
typedef struct {int eye_x,eye_y,eye_z,pitch,yaw;} OverheadProjection225;
static bool client_overhead_project225(void *user,const PathingEntity *actor,int *x,int *y) {
    OverheadProjection225 *p=user;
    if(actor->x<128 || actor->z<128 || actor->x>13056 || actor->z>13056)return false;
    int height=camera_get_height(sync_ctx.current_plane,actor->x,actor->z,world3d->heightmap,world->tile_flags);
    if(!project_from_point(actor->x,height-actor->height,actor->z,p->eye_x,p->eye_y,p->eye_z,p->pitch,p->yaw,x,y))return false;
    *x-=VIEWPORT_X;*y-=VIEWPORT_Y;return *x>=0;
}
#endif

bool client_draw(void) {
    client_login_dns_poll225();
#if RS2_SCENE225_RUNTIME
    int interface_delta225=game_ticks-interface_draw_cycle225;interface_draw_cycle225=game_ticks;
#endif

#if RS2_SCENE225_RUNTIME
    /* Original pushPlayers clears a reached approach destination during drawing. */
    if(has_minimap && player.base.x/128==minimap.flag_scene_tile_x && player.base.z/128==minimap.flag_scene_tile_z)
        minimap.flag_scene_tile_x=0;
#endif

    // Viewport camera parameters for this frame
#if RS2_SCENE225_RUNTIME
    if(!live_server || streamer.scene_state==2)
        camera_prepare_draw(&cam,player.base.x,player.base.z,sync_ctx.current_plane,world3d->heightmap,world->tile_flags);
#endif
    Camera draw_camera=cam;
    if(!live_server || streamer.scene_state==2)camera_shake225(&cam,&draw_camera);
    int camera_pitch = draw_camera.render_pitch;
    int camera_yaw   = draw_camera.render_yaw;
    int eye_x = draw_camera.eye_x;
    int eye_y = draw_camera.eye_y;
    int eye_z = draw_camera.eye_z;
    int ground_y = camera_get_height(sync_ctx.current_plane, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
    player.base.y = ground_y;

#if RS2_SCENE225_RUNTIME
    if(trace && ingame && getenv("RS2_TRACE_ACTORS"))printf("[LOCAL225 STATE] frame=%d pos=%d,%d yaw=%d,%d seq=%d,%d,%d primary=%d,%d,%d camera=%d,%d,%d,%d,%d\n",
        rendered_frames,player.base.x,player.base.z,player.base.yaw,player.base.dst_yaw,
        player.base.secondary_seq_id,player.base.secondary_seq_frame,player.base.secondary_seq_cycle,
        player.base.primary_seq_id,player.base.primary_seq_frame,player.base.primary_seq_cycle,
        cam.eye_x,cam.eye_y,cam.eye_z,cam.render_pitch,cam.render_yaw);
#endif
    int cross_frame=click_feedback_frame(&click_feedback);
    bool capture_cross=capture_clicks && cross_frame>=0 &&
        (cross_frame!=last_captured_cross || click_feedback.cycle<last_cross_cycle);
    last_captured_cross=cross_frame; last_cross_cycle=click_feedback.cycle;
    capture_this_frame = capture_path && (rendered_frames == capture_frame - 1 || capture_cross ||
        (capture_interval && (rendered_frames + 1) % capture_interval == 0));
        frame_capture_path = capture_path;
    if (capture_this_frame && rendered_frames != capture_frame - 1) {
        int length=snprintf(interval_path,sizeof(interval_path),"%s.frame-%06d.bmp",capture_path,rendered_frames+1);
        if(length<0 || (size_t)length>=sizeof(interval_path)) {fprintf(stderr,"Capture path too long\n");return false;}
        frame_capture_path=interval_path;
    }

#if RS2_SCENE225_RUNTIME
    if(trace && (capture_this_frame || getenv("RS2_TRACE_ACTORS")))for(int i=0;i<sync_ctx.npc_count;i++) {
        int slot=sync_ctx.npc_indices[i];NpcEntity *n=sync_ctx.npcs[slot];if(!n)continue;
        printf("[NPC225 STATE] frame=%d slot=%d pos=%d,%d yaw=%d,%d seq=%d,%d,%d primary=%d,%d,%d\n",
            rendered_frames,slot,n->base.x,n->base.z,n->base.yaw,n->base.dst_yaw,
            n->base.secondary_seq_id,n->base.secondary_seq_frame,n->base.secondary_seq_cycle,
            n->base.primary_seq_id,n->base.primary_seq_frame,n->base.primary_seq_cycle);
    }
#endif
    // Clear Framebuffer
    pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
    if (!ingame) {
        // client.draw() -> drawTitleScreen; the flame thread updates imageTitle0/1.
        title_screen_run_flames(&title, plat.now_ms(), game_ticks);
        title_screen_draw(&title, &font_b12, game_ticks);
        return true;
    }
    pix2d_cls();
    // client.drawScene: CYCLELOGIC2, buffered until the next updateGame (client.java:2186).
    if (live_server && streamer.scene_state == 2 && ++cyclelogic2 > 1802) { cyclelogic2 = 0; protocol_send_cyclelogic2(&session); }

    bool scene_ready=true;
#if RS2_SCENE225_RUNTIME
    scene_ready=!live_server || streamer.scene_state==2;
#endif
#if RS2_SCENE225_RUNTIME
    uint64_t texture_cycle225=pix3d225_texture_cache_cycle();
#endif
    // 1. Render 3D Viewport (0..512, 0..334)
    pix2d_set_clip(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y + VIEWPORT_H);
    pix3d_center_x = VIEWPORT_X + VIEWPORT_W / 2;
    pix3d_center_y = VIEWPORT_Y + VIEWPORT_H / 2;
    mouse_picker_reset(&picker, mouse_x, mouse_y);
    if (pending_click) {
#if RS2_SCENE225_RUNTIME
        world3d_click(world3d,pending_pick_x225,pending_pick_y225);
#else
        world3d_click(world3d, pending_click_x, pending_click_y);
#endif
    }
    if(scene_ready) {
#if RS2_SCENE225_RUNTIME
    if(scene_draw_cycle225==INT32_MAX)scene_draw_cycle225=INT32_MIN;else scene_draw_cycle225++;
#endif
    location_anim_tick(world3d,&loc_cfg,game_ticks-location_anim_cycle);
    location_anim_cycle=game_ticks;
#if RS2_SCENE225_RUNTIME
    if(!client_scene225(eye_x,eye_y,eye_z,camera_pitch,camera_yaw))return false;
#else
    world3d_render(world3d,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,0,sync_ctx.current_plane);
#endif
    // Optional replay helpers resolve pointers from actual received models;
    // they only enqueue client input and never create items or outcomes.
#if !RS2_SCENE225_RUNTIME
    if(verification_loc_id>=0)for(int x=0;x<104 && verification_loc_id>=0;x++)for(int z=0;z<104 && verification_loc_id>=0;z++) {
        GroundTile *tile=&world3d->tiles[sync_ctx.current_plane][x][z];
        for(int k=0;k<tile->loc_count && verification_loc_id>=0;k++) {
            WorldLocation *loc=&tile->locs[k];if(((loc->bitset>>14)&32767)!=verification_loc_id || !loc->model)continue;
            WorldLocationDraw draws[2];int draw_count=world3d_location_draws(loc,eye_x,eye_z,draws);
            for(int part=0;part<draw_count && verification_loc_id>=0;part++) {WorldLocationDraw draw=draws[part];
            int min_y=0,max_y=0;
            for(int v=0;v<loc->model->num_points;v++){if(loc->model->point_y[v]<min_y)min_y=loc->model->point_y[v];if(loc->model->point_y[v]>max_y)max_y=loc->model->point_y[v];}
            int sx,sy;
            if(!project_from_point(draw.x,draw.y+(min_y+max_y)/2,draw.z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&sx,&sy))continue;
            int best=100000,bx=-1,by=-1;
            for(int dy=-30;dy<=30;dy+=2)for(int dx=-24;dx<=24;dx+=2) {
                int px=sx+dx,py=sy+dy,score=dx*dx+dy*dy;
                if(score>=best || px<VIEWPORT_X || px>=VIEWPORT_X+VIEWPORT_W || py<VIEWPORT_Y || py>=VIEWPORT_Y+VIEWPORT_H)continue;
                if(model_draw_pickable(loc->model,draw.yaw,camera_pitch,camera_yaw,draw.x-eye_x,draw.y-eye_y,draw.z-eye_z,px,py)){best=score;bx=px;by=py;}
            }
            if(bx>=0) {
                ClientEvent pointer;memset(&pointer,0,sizeof(pointer));pointer.type=CLIENT_EVENT_MOUSE_MOVE;pointer.x=bx;pointer.y=by;client_queue_event(&pointer);
                verification_cursor_x=bx;verification_cursor_y=by;verification_loc_id=-1;
                printf("[INPUT] rendered loc=%d world=(%d,%d,%d) pointer=(%d,%d)\n",(loc->bitset>>14)&32767,sync_ctx.current_plane,streamer.base_tile_x+x,streamer.base_tile_z+z,bx,by);
            }
            }
        }
    }
    if(verification_obj_id>=0) for(int x=0;x<104 && verification_obj_id>=0;x++)
        for(int z=0;z<104 && verification_obj_id>=0;z++) {
            GroundTile *tile=&world3d->tiles[sync_ctx.current_plane][x][z];
            for(GroundItem *item=tile->items;item && verification_obj_id>=0;item=item->next) {
                if(item->id!=verification_obj_id || !item->model) continue;
                int y=(world3d->heightmap[sync_ctx.current_plane][x][z]+world3d->heightmap[sync_ctx.current_plane][x+1][z]+
                    world3d->heightmap[sync_ctx.current_plane][x][z+1]+world3d->heightmap[sync_ctx.current_plane][x+1][z+1])/4;
                int raise=0;for(int k=0;k<tile->loc_count;k++) if(tile->locs[k].shape>=9 && tile->locs[k].shape<=21 &&
                    tile->locs[k].model && tile->locs[k].model->obj_raise>raise) raise=tile->locs[k].model->obj_raise;
                y-=raise;int sx,sy;
                if(!project_from_point(x*128+64,y-16,z*128+64,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&sx,&sy)) continue;
                for(int dy=-36;dy<=36 && verification_obj_id>=0;dy+=2) for(int dx=-36;dx<=36 && verification_obj_id>=0;dx+=2) {
                    int px=sx+dx,py=sy+dy;
                    if(px<8 || px>=520 || py<11 || py>=345) continue;
                    if(model_draw_pickable(item->model,0,camera_pitch,camera_yaw,x*128+64-eye_x,y-eye_y,z*128+64-eye_z,px,py)) {
                        ClientEvent pointer;memset(&pointer,0,sizeof(pointer));pointer.type=CLIENT_EVENT_MOUSE_MOVE;
                        pointer.x=px;pointer.y=py;client_queue_event(&pointer);
                        verification_cursor_x=px;verification_cursor_y=py;verification_obj_id=-1;
                        printf("[INPUT] rendered ground item=%d world=(%d,%d) pointer=(%d,%d)\n",item->id,
                            streamer.base_tile_x+x,streamer.base_tile_z+z,px,py);
                    }
                }
            }
        }
 #endif
    if (capture_this_frame) {
        char terrain_path[2048];
#if RS2_SCENE225_RUNTIME
        if(has_minimap && minimap.image_minimap) {
            snprintf(terrain_path,sizeof(terrain_path),"%s.minimap.bmp",frame_capture_path);
            if(!plat.save_bmp(terrain_path,minimap.image_minimap->pixels,512,512))fprintf(stderr,"Minimap capture failed\n");
        }
#endif
        snprintf(terrain_path, sizeof(terrain_path), "%s.terrain.bmp", frame_capture_path);
        if (!plat.save_bmp(terrain_path, pixels, CLIENT_WIDTH, CLIENT_HEIGHT)) {
            fprintf(stderr, "Terrain capture failed\n");
            return false;
        }
        printf("[CAPTURE] camera=(%d,%d,%d) pitch=%d yaw=%d -> %s\n", eye_x, eye_y, eye_z,
            camera_pitch, camera_yaw, terrain_path);
        printf("[VERIFY] player tile=(%d,%d) sequence=%d frame=%d\n", player.base.x / 128,
            player.base.z / 128, player.base.secondary_seq_id, player.base.secondary_seq_frame);
        if(trace) printf("[NAV] frame=%d player_world=(%d,%d) destination_local=(%d,%d) yaw=%d\n",rendered_frames,
            streamer.base_tile_x+player.base.x/128,streamer.base_tile_z+player.base.z/128,
            has_minimap?minimap.flag_scene_tile_x:0,has_minimap?minimap.flag_scene_tile_z:0,camera_yaw);
        if(trace) printf("[STATE] live=%d plane=%d npcs=%d players=%d appearance_weapon=%d hash=%llu\n",live_server,sync_ctx.current_plane,
            sync_ctx.npc_count,sync_ctx.player_count,player.appearances[3],(unsigned long long)player.appearance_hashcode);
        if(trace) for(int i=0;i<sync_ctx.npc_count;i++) {
            int slot=sync_ctx.npc_indices[i];NpcEntity *n=sync_ctx.npcs[slot];if(!n || !n->has_type) continue;
            printf("[ENTITY] npc slot=%d type=%d name=%s world=(%d,%d) minimap=%d\n",slot,n->type.index,n->type.name,
                streamer.base_tile_x+n->base.x/128,streamer.base_tile_z+n->base.z/128,n->type.minimap);
        }

    }

#if RS2_SCENE225_RUNTIME
    /* drawScene only resolves the clicked ground. updateGame consumes it on
     * the following cycle, after advancing the old cross animation. */
    pending_click=false;
#else
    if (pending_click) {
        if (world3d->click_tile_x != -1) {
            PathResult res;
            if (pathfinder_find_path(&pathfinder, &collision_maps[sync_ctx.current_plane],
                                     player.base.path_tile_x[0], player.base.path_tile_z[0],
                                     world3d->click_tile_x, world3d->click_tile_z, true, &res)) {
                // Original client waits for PLAYER_INFO movement. Queueing
                // the clicked path here adds the server deltas a second time.
                if(!live_server) pathing_entity_queue_path(&player.base,res.steps_x,res.steps_z,res.step_count,false);
                if(live_server) {
                        bool sent=protocol_send_route(&session,streamer.base_tile_x,streamer.base_tile_z,player.base.path_tile_x[0],player.base.path_tile_z[0],
                            res.steps_x,res.steps_z,res.step_count,false);
                        if(!sent) fprintf(stderr,"[ACTION] Route send failed\n");
                        if(sent) click_feedback_start(&click_feedback,CLICK_WALK,pending_click_x,pending_click_y);
                        if (sent && has_minimap && res.step_count > 0) {
                            minimap.flag_scene_tile_x = res.steps_x[res.step_count-1];
                            minimap.flag_scene_tile_z = res.steps_z[res.step_count-1];
                        }
                        if(trace && res.step_count>0) printf("[ACTION] route %s destination=(%d,%d)\n",sent?"sent":"failed",
                            streamer.base_tile_x+res.steps_x[res.step_count-1],streamer.base_tile_z+res.steps_z[res.step_count-1]);
                    }
                if(!live_server) click_feedback_start(&click_feedback,CLICK_WALK,pending_click_x,pending_click_y);
            }
        }
        world3d->taking_input = false;
        pending_click = false;
    }
#endif

#if !RS2_SCENE225_RUNTIME
    // Render Entities inside viewport with full camera rotation
    arena_reset(&arena_frame);

    if (offline) {
        Model *h_model = npc_entity_get_animated_model(&hans,&seq_cfg,&arena_frame);
        if (h_model) {
            int hx = hans.base.x - eye_x;
            int hy = world3d->heightmap[0][hans.base.path_tile_x[0]][hans.base.path_tile_z[0]] - eye_y;
            int hz = hans.base.z - eye_z;
            bool hit_hans = model_draw_pickable(h_model, 0, camera_pitch, camera_yaw, hx, hy, hz, mouse_x, mouse_y);
            if (hit_hans) {
                uint32_t bs = mouse_picker_make_bitset(1, 0, hans.base.path_tile_x[0], hans.base.path_tile_z[0]);
                mouse_picker_add_picked(&picker, bs);
            }
        }
    }

    if(live_server) {
        for(int index=0;index<sync_ctx.npc_count;index++) {
            int slot=sync_ctx.npc_indices[index];NpcEntity *npc=sync_ctx.npcs[slot];
            if(!npc || !npc->has_type) continue;
            int type=npc->type.index;if(type<0 || type>=npc_cfg.count) continue;
            size_t mark=arena_frame.offset;Model *model=npc_entity_get_animated_model(npc,&seq_cfg,&arena_frame);
            if(model) {
                model=actor_with_spotanim(model,&npc->base,&spot_cfg,&arena_frame);
                int y=camera_get_height(sync_ctx.current_plane,npc->base.x,npc->base.z,world3d->heightmap,world->tile_flags);
                if(verification_npc_type==type) {
                    int64_t distance=(int64_t)(npc->base.x-player.base.x)*(npc->base.x-player.base.x)+(int64_t)(npc->base.z-player.base.z)*(npc->base.z-player.base.z);
                    bool nearest=true;
                    for(int n=0;n<sync_ctx.npc_count;n++) {NpcEntity *candidate=sync_ctx.npcs[sync_ctx.npc_indices[n]];
                        if(!candidate || !candidate->has_type || candidate->type.index!=type)continue;
                        int64_t d=(int64_t)(candidate->base.x-player.base.x)*(candidate->base.x-player.base.x)+(int64_t)(candidate->base.z-player.base.z)*(candidate->base.z-player.base.z);
                        if(d<distance){nearest=false;break;}
                    }
                    int sx,sy;
                    if(nearest && project_from_point(npc->base.x,y-npc->base.height/2,npc->base.z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&sx,&sy)) {
                        int best=100000,bx=-1,by=-1;
                        for(int dy=-20;dy<=20;dy+=2) for(int dx=-16;dx<=16;dx+=2) {
                            int px=sx+dx,py=sy+dy,score=dx*dx+dy*dy;
                            if(score>=best || px<VIEWPORT_X || px>=VIEWPORT_X+VIEWPORT_W || py<VIEWPORT_Y || py>=VIEWPORT_Y+VIEWPORT_H)continue;
                            if(model_draw_pickable(model,npc->base.yaw,camera_pitch,camera_yaw,npc->base.x-eye_x,y-eye_y,npc->base.z-eye_z,px,py)){best=score;bx=px;by=py;}
                        }
                        if(bx>=0) {
                            ClientEvent pointer;memset(&pointer,0,sizeof(pointer));pointer.type=CLIENT_EVENT_MOUSE_MOVE;
                            pointer.x=bx;pointer.y=by;client_queue_event(&pointer);
                            verification_cursor_x=bx;verification_cursor_y=by;verification_npc_type=-1;
                            printf("[INPUT] rendered NPC type=%d slot=%d pointer=(%d,%d)\n",type,slot,bx,by);
                        }
                    }
                }
                if(model_draw_pickable(model,npc->base.yaw,camera_pitch,camera_yaw,npc->base.x-eye_x,y-eye_y,npc->base.z-eye_z,mouse_x,mouse_y))
                    mouse_picker_add_picked(&picker,mouse_picker_make_bitset(1,slot,npc->base.x/128,npc->base.z/128));
            }
            arena_frame.offset=mark;
        }
        for(int index=0;index<sync_ctx.player_count;index++) {
            int slot=sync_ctx.player_indices[index];PlayerEntity *other=sync_ctx.players[slot];
            if(!other || !other->visible) continue;
            size_t mark=arena_frame.offset;Model *model=player_entity_get_model(other,&idk_cfg,&obj_cfg,&seq_cfg,&arena_frame);
            if(model) {
                model=actor_with_spotanim(model,&other->base,&spot_cfg,&arena_frame);
                int y=camera_get_height(sync_ctx.current_plane,other->base.x,other->base.z,world3d->heightmap,world->tile_flags);
                model=player_with_location(model,other,game_ticks,y,&arena_frame);
                if(model_draw_pickable(model,other->base.yaw,camera_pitch,camera_yaw,other->base.x-eye_x,y-eye_y,other->base.z-eye_z,mouse_x,mouse_y))
                    mouse_picker_add_picked(&picker,mouse_picker_make_bitset(0,slot,other->base.x/128,other->base.z/128));
            }
            arena_frame.offset=mark;
        }
    }

    // Render Player (authentic walk/stand animation progression)
    int p_frame_id = has_anim ? current_entity_frame(&player.base,&seq_cfg):-1;
    if(trace && capture_this_frame)printf("[ANIM] draw local primary=%d frame=%d delay=%d transform=%d game_cycle=%d\n",player.base.primary_seq_id,player.base.primary_seq_frame,player.base.primary_seq_delay,p_frame_id,game_ticks);

    if (player.visible) {
        Model *p_model = player_entity_get_model(&player,&idk_cfg,&obj_cfg,&seq_cfg,&arena_frame);
        if (p_model) {
            p_model=actor_with_spotanim(p_model,&player.base,&spot_cfg,&arena_frame);
            p_model=player_with_location(p_model,&player,game_ticks,ground_y,&arena_frame);
            int px = player.base.x - eye_x;
            int py = ground_y - eye_y;
            int pz = player.base.z - eye_z;
            model_draw(p_model, player.base.yaw, camera_pitch, camera_yaw, px, py, pz);
        }
    }

    effects_draw(&effects,&arena_frame,sync_ctx.current_plane,eye_x,eye_y,eye_z,camera_pitch,camera_yaw);
#endif
    pix3d_end_scene();
#if RS2_SCENE225_RUNTIME
    if(!pix3d225_texture_cache_animate_used(texture_cycle225,interface_delta225)) {
        fprintf(stderr,"Cannot animate original texture indices\n");return false;
    }
#endif
    // Original entity overlays, using the received hit/health expiry timers.
    for(int index=-1;index<sync_ctx.npc_count+sync_ctx.player_count;index++) {
        PathingEntity *actor=NULL;
        if(index<0)actor=&player.base;
        else if(index<sync_ctx.npc_count) {NpcEntity *n=sync_ctx.npcs[sync_ctx.npc_indices[index]];if(n)actor=&n->base;}
        else {PlayerEntity *p=sync_ctx.players[sync_ctx.player_indices[index-sync_ctx.npc_count]];if(p && p->visible)actor=&p->base;}
        if(!actor || actor->combat_cycle<=game_ticks+100)continue;
        int y=camera_get_height(sync_ctx.current_plane,actor->x,actor->z,world3d->heightmap,world->tile_flags);
        int bx=-1,by=-1,hx=-1,hy=-1;
        project_from_point(actor->x,y-actor->height-15,actor->z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&bx,&by);
        project_from_point(actor->x,y-actor->height/2,actor->z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&hx,&hy);
        combat_feedback_draw(actor,game_ticks,&media,&font_p11,bx,by,hx,hy);
    }

    // Authentic Rev 225 Hint Arrow Rendering (Packet 25)
    if (media.headicons[2] && (game_ticks % 20 < 10)) {
        if (hint_type == 1 && live_server) {
            if (hint_npc >= 0 && hint_npc < MAX_NPCS && sync_ctx.npcs[hint_npc]) {
                NpcEntity *target_npc = sync_ctx.npcs[hint_npc];
                int wx = target_npc->base.x;
                int wz = target_npc->base.z;
                int gy = camera_get_height(sync_ctx.current_plane, wx, wz, world3d->heightmap, world->tile_flags);
                /* An NPC not yet drawn keeps original default height0. */
                int h = RS2_SCENE225_RUNTIME ? target_npc->base.height : (target_npc->base.height > 0 ? target_npc->base.height : 200);
                int wy = gy - h - 15;
                int sx, sy;
                if (project_from_point(wx, wy, wz, eye_x, eye_y, eye_z, camera_pitch, camera_yaw, &sx, &sy)) {
                    if (sx >= VIEWPORT_X && sx < VIEWPORT_X + VIEWPORT_W &&
                        sy >= VIEWPORT_Y && sy < VIEWPORT_Y + VIEWPORT_H) {
                        pix32_draw(media.headicons[2], sx - 12, sy - 28);
                    }
                }
            }
        } else if (hint_type == 2) {
            int tile_x_local = hint_tile_x - streamer.base_tile_x;
            int tile_z_local = hint_tile_z - streamer.base_tile_z;
            if (tile_x_local >= 0 && tile_x_local < 104 && tile_z_local >= 0 && tile_z_local < 104) {
                int wx = tile_x_local * 128 + hint_offset_x;
                int wz = tile_z_local * 128 + hint_offset_z;
                int gy = camera_get_height(sync_ctx.current_plane, wx, wz, world3d->heightmap, world->tile_flags);
                int wy = gy - (hint_height * 2);
                int sx, sy;
                if (project_from_point(wx, wy, wz, eye_x, eye_y, eye_z, camera_pitch, camera_yaw, &sx, &sy)) {
                    if (sx >= VIEWPORT_X && sx < VIEWPORT_X + VIEWPORT_W &&
                        sy >= VIEWPORT_Y && sy < VIEWPORT_Y + VIEWPORT_H) {
                        pix32_draw(media.headicons[2], sx - 12, sy - 28);
                    }
                }
            }
        } else if (hint_type == 10 && live_server) {
            if (hint_player >= 0 && hint_player < MAX_PLAYERS && sync_ctx.players[hint_player]) {
                PlayerEntity *target_pl = sync_ctx.players[hint_player];
                int wx = target_pl->base.x;
                int wz = target_pl->base.z;
                int gy = camera_get_height(sync_ctx.current_plane, wx, wz, world3d->heightmap, world->tile_flags);
                int h = RS2_SCENE225_RUNTIME ? target_pl->base.height : (target_pl->base.height > 0 ? target_pl->base.height : 200);
                int wy = gy - h - 15;
                int sx, sy;
                if (project_from_point(wx, wy, wz, eye_x, eye_y, eye_z, camera_pitch, camera_yaw, &sx, &sy)) {
                    if (sx >= VIEWPORT_X && sx < VIEWPORT_X + VIEWPORT_W &&
                        sy >= VIEWPORT_Y && sy < VIEWPORT_Y + VIEWPORT_H) {
                        pix32_draw(media.headicons[2], sx - 12, sy - 28);
                    }
                }
            }
        }
    }

    } else {
#if RS2_SCENE225_RUNTIME
        location_anim_cycle=game_ticks;
        for(int y=0;y<VIEWPORT_H;y++)memcpy(pixels+(VIEWPORT_Y+y)*CLIENT_WIDTH+VIEWPORT_X,
            loading_viewport225+y*VIEWPORT_W,VIEWPORT_W*sizeof(*pixels));
#endif
    }
#if RS2_SCENE225_RUNTIME
    if(scene_ready && has_fonts) {
        OverheadProjection225 projection={eye_x,eye_y,eye_z,camera_pitch,camera_yaw};
        int chat_effects=0;
        for(int id=0;id<texture_varp_types.count;id++)if(varp_type_clientcode(&texture_varp_types,id)==6)chat_effects=varp_get(&varp_ctx,id);
        chatbox.public_mode=public_chat_setting;
        if(!overhead_chat_draw(&overhead_chat225,&sync_ctx,&chatbox,&font_b12,client_overhead_project225,&projection,scene_draw_cycle225,chat_effects,VIEWPORT_X,VIEWPORT_Y) && trace)
            fprintf(stderr,"[CHAT] Overhead list rejected invalid/original overflow state\n");
    }
#endif
    /* Original updateVarp clientcode8 controls split private chat. Read the same
     * current varp for server updates and local interface predictions. */
    chatbox.public_mode = public_chat_setting;
    chatbox.private_mode = private_chat_setting;
    chatbox.trade_mode = trade_chat_setting;
    for (int id = 0; id < texture_varp_types.count; id++)
        if (varp_type_clientcode(&texture_varp_types, id) == 8)
            chatbox.split_private = varp_get(&varp_ctx, id);
    if (has_fonts && streamer.scene_state == 2) {
        pix2d_set_clip(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y + VIEWPORT_H);
        chatbox_draw_private(&chatbox, VIEWPORT_X, VIEWPORT_Y, &font_p12, system_update_timer);
#if RS2_SCENE225_RUNTIME
        /* client.drawWildyLevel refreshes overrideChat during scene draw;
         * incoming packets consume the last drawn value, including rebuilds. */
        private_override_chat225=client_override_chat225();
#endif
        pix2d_reset_clip();
    }
    for(int i=0;i<comp_cfg.count;i++){chatbox_prepare_friend_component(&chatbox,&comp_cfg.instances[i],10);chatbox_prepare_report_component(&chatbox,&comp_cfg.instances[i],session.is_mod,game_ticks);last_login_prepare225(&last_login225,&comp_cfg.instances[i]);}
    // Dynamically build context menu & resolve primary hover action
    if (!menu.visible && !(comp_cfg.drag.active && comp_cfg.drag.threshold && comp_cfg.drag.cycles >= 5)) {
        context_menu_clear(&menu);
#if RS2_SCENE225_RUNTIME
        /* Original menu construction begins with Cancel for interfaces too. */
        context_menu_add_action(&menu,"Cancel",(MenuAction){.type=MENU_CANCEL});
#endif

    chatbox_build_menu(&chatbox, &menu, mouse_x, mouse_y, true, session.is_mod, system_update_timer);
    if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
        Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
        if (modal_com) {
            int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
            int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
            int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
            int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
            component_build_menu(&comp_cfg, modal_com, mouse_x, mouse_y, mx_pos, my_pos, 0, &menu, &obj_cfg);
        }
    } else if (mouse_x >= VIEWPORT_X && mouse_x < VIEWPORT_X + VIEWPORT_W && mouse_y >= VIEWPORT_Y && mouse_y < VIEWPORT_Y + VIEWPORT_H) {
#if RS2_SCENE225_RUNTIME
        if(live_server) {
            picker.location_present225=client_location_present225;
            picker.location_user225=scene_runtime?world3d_scene225(scene_runtime225_build(scene_runtime)):NULL;
            picker.world_x225 = streamer.base_tile_x + (player.base.x >> 7);
            picker.world_z225 = streamer.base_tile_z + (player.base.z >> 7);
            mouse_picker_append_live_menu225(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&sync_ctx,client_override_chat225());
        }
#else
        if(live_server) mouse_picker_build_live_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&sync_ctx);
#endif
        else if(offline) mouse_picker_build_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&hans,1,&player,1,player.combat_level);
        else context_menu_add_action(&menu,"Walk here",(MenuAction){.type=MENU_CANCEL});
    } else if (mouse_x >= 562 && mouse_x < 752 && mouse_y >= 231 && mouse_y < 492) {
        if (has_components && selected_tab >= 0 && selected_tab < 14) {
            int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
            if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
            if (tab_if > 0 && tab_if != 65535) {
                Component *tab_com = component_get(&comp_cfg, tab_if);
                if (tab_com) {
                    component_build_menu(&comp_cfg, tab_com, mouse_x, mouse_y, 562, 231, 0, &menu, &obj_cfg);
                }
            }
        }
    } else if (mouse_x >= 22 && mouse_x < 501 && mouse_y >= 375 && mouse_y < 471) {
        int active_chat_if = interfaces.chat;
        if (active_chat_if != -1) {
            Component *chat_com = component_get(&comp_cfg, active_chat_if);
            if (chat_com) {
                component_build_menu(&comp_cfg, chat_com, mouse_x, mouse_y, 22, 375, 0, &menu, &obj_cfg);
            }
        } else {
            chatbox_build_menu(&chatbox, &menu, mouse_x, mouse_y, false, session.is_mod, system_update_timer);
        }
    }

    }

    if(verification_inventory_slot>=0 && !menu.visible && selected_tab==3 && interfaces.sidebar==-1) {
        int root_id=tab_interface_id[3];Component *root=component_get(&comp_cfg,root_id);
        for(int y=233;y<491 && verification_inventory_slot>=0;y+=4)
            for(int x=564;x<751 && verification_inventory_slot>=0;x+=4) {
                int slot=-1;Component *grid=root?component_find_inv_slot(&comp_cfg,root,x,y,562,231,0,&slot):NULL;
                if(!grid || grid->id!=3214 || slot!=verification_inventory_slot || !grid->inv_slot_obj_id || grid->inv_slot_obj_id[slot]<=0) continue;
                ClientEvent pointer;memset(&pointer,0,sizeof(pointer));pointer.type=CLIENT_EVENT_MOUSE_MOVE;
                pointer.x=x;pointer.y=y;client_queue_event(&pointer);
                verification_cursor_x=x;verification_cursor_y=y;verification_inventory_slot=-1;
                printf("[INPUT] inventory slot=%d item=%d pointer=(%d,%d)\n",slot,grid->inv_slot_obj_id[slot]-1,x,y);
            }
    }
    context_menu_prioritize(&menu);
    if(verification_option[0] && menu.visible) {
        for(int index=menu.size-1;index>=0;index--) {
            if(strncmp(menu.options[index],verification_option,strlen(verification_option))) continue;
            ClientEvent click;memset(&click,0,sizeof(click));click.type=CLIENT_EVENT_MOUSE_DOWN;click.button=CLIENT_BUTTON_LEFT;
            click.x=menu.x+menu.width/2;click.y=menu.y+18+(menu.size-1-index)*15+7;
            client_queue_event(&click);
            printf("[INPUT] rendered menu option=%s pointer=(%d,%d)\n",menu.options[index],click.x,click.y);
            verification_option[0]=0;break;
        }
    }
    static char tooltip_buf[128];
    const char *primary_action = "";
    if(menu.size<2 && (selection.item_selected || selection.spell_selected)) {
        if(selection.item_selected) snprintf(tooltip_buf,sizeof(tooltip_buf),"Use %.64s with...",selection.name);
        else snprintf(tooltip_buf,sizeof(tooltip_buf),"%.120s...",selection.spell_caption);
        primary_action=tooltip_buf;
    } else if (menu.size > 0) {
        const char *top_opt = menu.options[menu.size - 1];
        if (menu.size > 2) {
            snprintf(tooltip_buf, sizeof(tooltip_buf), RS2_SCENE225_RUNTIME?"%s@whi@ / %d more options":"%s @whi@/ %d more options", top_opt, menu.size - 2);
            primary_action = tooltip_buf;
        } else {
            primary_action = top_opt;
        }
    }

    // Reset clip to full window
    pix2d_reset_clip();
    pix3d_center_x = CLIENT_WIDTH / 2;
    pix3d_center_y = CLIENT_HEIGHT / 2;

    if(has_media) click_feedback_draw(&click_feedback,&media,VIEWPORT_X,VIEWPORT_Y,VIEWPORT_W,VIEWPORT_H);

    // 2. Draw authentic 2004scape UI Shell & Frame Artwork
    if (has_media) {
        media_draw_shell(&media);
        if (has_minimap && scene_ready) {
            int map_yaw = (cam.orbit_yaw + minimap_anticheat_angle) & 0x7FF;
            minimap_draw(&minimap, &media, player.base.x, player.base.z, cam.orbit_yaw, minimap_anticheat_angle, minimap_zoom, 561, 5);
            for(int x=0;x<world3d->max_tile_x;x++) for(int z=0;z<world3d->max_tile_z;z++)
                if(world3d->tiles[sync_ctx.current_plane][x][z].items)
                    minimap_draw_dot(player.base.x,player.base.z,x*128+64,z*128+64,map_yaw,minimap_zoom,561,5,media.mapdots[0],media.mapback);
            if(live_server) for(int i=0;i<sync_ctx.npc_count;i++) {
                NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];
                minimap_draw_npc_dot(player.base.x,player.base.z,npc,map_yaw,minimap_zoom,561,5,media.mapdots[1],media.mapback);
            }
            if(offline) minimap_draw_npc_dot(player.base.x,player.base.z,&hans,map_yaw,minimap_zoom,561,5,media.mapdots[1],media.mapback);
            if(live_server) for(int i=0;i<sync_ctx.player_count;i++) {
                PlayerEntity *other=sync_ctx.players[sync_ctx.player_indices[i]];
                minimap_draw_player_dot(player.base.x,player.base.z,other,map_yaw,minimap_zoom,561,5,media.mapdots[2],media.mapback);
            }
            minimap_draw_destination(&minimap,&media,player.base.x,player.base.z,map_yaw,minimap_zoom,561,5);
            // Original minimap draws the local centre marker last.
            pix2d_fill_rect(561+93,5+82,3,3,0xFFFFFF);
        }
#if RS2_SCENE225_RUNTIME
        /* drawGame leaves areaMapback intact while sceneState is loading. */
        if(!scene_ready && loading_mapback_valid225)for(int y=0;y<160;y++)
            memcpy(pixels+(5+y)*CLIENT_WIDTH+561,loading_mapback225+y*168,168*sizeof(*pixels));
#endif
        media_draw_tabs_flashing(&media, selected_tab, dev_tabs_unlocked ? NULL : tab_interface_id, flashing_tab, game_ticks);

        // Draw interactive Chatbox or Dialogue Interface
        if(chatbox.private_prompt_open && has_fonts) {
            chatbox_draw_private_prompt(&chatbox,22,375,&font_b12);
        } else if (chatbox.count_dialog_open && has_fonts) {
            chatbox_draw(&chatbox,NULL,22,375,&font_b12);
#if RS2_SCENE225_RUNTIME
        } else if (modal_message225[0] && has_fonts) {
            pixfont_draw_string_center(&font_b12,261,415,0,modal_message225);
            pixfont_draw_string_center(&font_b12,261,435,128,"Click to continue");
#endif
        } else if (interfaces.chat != -1 && has_components) {
            Component *chat_com = component_get(&comp_cfg, interfaces.chat);
            if (chat_com) {
#if RS2_SCENE225_RUNTIME
                client_chat_interface225(chat_com,interface_delta225);
#else
                component_draw_with_varps(&comp_cfg, chat_com, 22, 375, 0, &varp_ctx);
#endif
            }
        } else if (sticky_chat_interface_id != -1 && has_components) {
            Component *tut_com = component_get(&comp_cfg, sticky_chat_interface_id);
            if (tut_com) {
                component_draw_with_varps(&comp_cfg, tut_com, 22, 375, 0, &varp_ctx);
            }
        } else if (has_fonts) {
#if RS2_SCENE225_RUNTIME
            chatbox_draw225(&chatbox,NULL,22,375,&font_p12,offline?NULL:title.username,media.scrollbar0,media.scrollbar1);
#else
            chatbox_draw(&chatbox, NULL, 22, 375, &font_p12);
#endif
        }

        // Draw active Sidebar Tab Interface
        if (has_components && selected_tab >= 0 && selected_tab < 14) {
            int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
            if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
            if (tab_if > 0 && tab_if != 65535) {
                Component *tab_com = component_get(&comp_cfg, tab_if);
                if (tab_com) {
                    #if RS2_SCENE225_RUNTIME
                    size_t model_mark225=arena_frame.offset;client_interface_pose225(tab_com,interface_delta225,false);
#endif
                    component_draw_with_varps(&comp_cfg, tab_com, 562, 231, 0, &varp_ctx);
#if RS2_SCENE225_RUNTIME
                    client_interface_pose225(tab_com,0,true);arena_frame.offset=model_mark225;
#endif
                }
            }
        }

        // Draw Main Screen Modal Interface (Bank 5292, Shop 300, etc.) if active
        if (scene_ready && interfaces.viewport != -1 && has_components) {
            Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
            if (modal_com) {
                int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
#if RS2_SCENE225_RUNTIME
                client_design_prepare225(&actions,modal_com,&arena_frame,game_ticks);
#else
                client_design_prepare(&actions,modal_com,&arena_frame,rendered_frames);
#endif
                #if RS2_SCENE225_RUNTIME
                (void)mx_pos;(void)my_pos;client_viewport_interface225(modal_com,interface_delta225);
#else
                component_draw_with_varps(&comp_cfg, modal_com, mx_pos, my_pos, 0, &varp_ctx);
#endif
            }
        }
        if(streamer.scene_state==2)combat_multiway_draw225(&media,sync_ctx.in_multizone,(player.base.x>>7)+streamer.base_tile_x,(player.base.z>>7)+streamer.base_tile_z,VIEWPORT_X,VIEWPORT_Y);
    } else {
        // Fallback border
        pix2d_draw_rect(VIEWPORT_X - 1, VIEWPORT_Y - 1, VIEWPORT_W + 2, VIEWPORT_H + 2, 0x473B2E);
    }

    // 3. Draw Authentic RS2 UI Text using Bitmap Fonts
#if RS2_SCENE225_RUNTIME
    if(trace && capture_this_frame)for(int row=0;row<menu.size;row++)
        printf("[MENU225] frame=%d row=%d type=%d target=%d label=%s\n",rendered_frames,row,menu.actions[row].type,menu.actions[row].target,menu.options[row]);
#endif
    if (has_fonts) {
        // Viewport hover text (top-left)
        if (scene_ready && (!RS2_SCENE225_RUNTIME || !menu.visible) && primary_action && primary_action[0] != '\0') {
#if RS2_SCENE225_RUNTIME
            if(menu.size>=2 || selection.item_selected || selection.spell_selected)
                pixfont_draw_tooltip225(&font_b12,VIEWPORT_X+4,VIEWPORT_Y+15,0xffffff,primary_action,game_ticks/1000);
#else
            pixfont_draw_string_taggable(&font_b12, 12, 24, 0xFFFFFF, primary_action, true);
#endif
        }

        // Chat filter button labels & status indicators (authentic RS2 backbase1)
        pixfont_draw_string_taggable_center(&font_p12, 57, 504, 0xFFFFFF, "Public chat", true);
        const char *pub_txt = (public_chat_setting == 0) ? "@gre@On" : (public_chat_setting == 1) ? "@yel@Friends" : (public_chat_setting == 2) ? "@red@Off" : "@cya@Hide";
        pixfont_draw_string_taggable_center(&font_p12, 57, 517, 0xFFFFFF, pub_txt, true);

        pixfont_draw_string_taggable_center(&font_p12, 186, 504, 0xFFFFFF, "Private chat", true);
        const char *priv_txt = (private_chat_setting == 0) ? "@gre@On" : (private_chat_setting == 1) ? "@yel@Friends" : "@red@Off";
        pixfont_draw_string_taggable_center(&font_p12, 186, 517, 0xFFFFFF, priv_txt, true);

        pixfont_draw_string_taggable_center(&font_p12, 326, 504, 0xFFFFFF, "Trade/duel", true);
        const char *trade_txt = (trade_chat_setting == 0) ? "@gre@On" : (trade_chat_setting == 1) ? "@yel@Friends" : "@red@Off";
        pixfont_draw_string_taggable_center(&font_p12, 326, 517, 0xFFFFFF, trade_txt, true);

        pixfont_draw_string_taggable_center(&font_p12, 462, 509, 0xFFFFFF, "Report abuse", true);
    }


    // 5. Draw Right-Click Context Menu (topmost layer)
    if (has_fonts && menu.visible) {
        context_menu_draw(&menu, mouse_x, mouse_y, &font_b12);
    }
    if (has_fonts && streamer.scene_state == 2) client_draw_system_update();

    return true;
}

void client_end_frame(void) {
    if (capture_this_frame && rendered_frames == capture_frame - 1) running = false;
    rendered_frames++;

    if (login_pending && running) client_login(false);
}

static void client_login(bool reconnect) {
        login_pending = false; reconnecting = false;
        if (trace) printf("[PARITY] login frame=%d random_state=%llu\n", rendered_frames, (unsigned long long)math_random_state());
        LoginResponse reply = protocol_login_ex(&session, "127.0.0.1", server_port, title.username, title.password, false, reconnect);
        printf("[LOGIN] %s reply=%d\n", reconnect ? "reconnect" : "login", (int)reply);
        if (reply == LOGIN_SUCCESS || reply == LOGIN_MOD_SUCCESS) {
            input_tracking_disable(&input_tracking);
            // client.login() success reset, then prepareGameScreen/unloadTitle.
            idle_timeout = 0; system_update_timer = 0; hint_type = 0;
            context_menu_clear(&menu); menu.visible = false;
            chatbox_free(&chatbox); chatbox_init(&chatbox, "Player");
#if RS2_SCENE225_RUNTIME
            snprintf(chatbox.username,sizeof(chatbox.username),"%.31s",title.username);
            jstring_format_name(chatbox.username);
            memset(loading_viewport225,0,sizeof(loading_viewport225));
#endif
            selection.item_selected = false; selection.spell_selected = false;
            streamer.scene_state = 0; scene_build_pending = false;
            audio_queue_clear225();
            idle_cycles = 0;
            // client.java:7182-7187.
            cam.anticheat_x = (int)(math_random() * 100.0) - 50;
            cam.anticheat_z = (int)(math_random() * 110.0) - 55;
            cam.anticheat_yaw = (int)(math_random() * 80.0) - 40;
            minimap_anticheat_angle = (int)(math_random() * 120.0) - 60;
            minimap_zoom = (int)(math_random() * 30.0) - 20;
            cam.orbit_yaw = ((int)(math_random() * 20.0) - 10) & 0x7FF;
            if (capture_yaw) cam.orbit_yaw = capture_yaw;   // --camera-yaw debug override
            if (has_minimap) { minimap.flag_scene_tile_x = 0; minimap.flag_scene_tile_z = 0; }
            entity_sync_cleanup(&sync_ctx);
            entity_sync_init(&sync_ctx, &npc_cfg);
            sync_ctx.trace = trace; sync_ctx.seq_cfg = &seq_cfg;
            player.visible = false;
            zones.local_pid = -1;
            effects_clear(&effects);
            zone_sync_reset(&zones);
            sticky_chat_interface_id = -1;
#if RS2_SCENE225_RUNTIME
            modal_message225[0]=0;
#endif
            interface_state_init(&interfaces);
            comp_cfg.pressed_continue=false;
            selected_tab = 3;
            flashing_tab = -1;
            design = (ClientDesign){0};
            comp_cfg.drag.active = false; pending_click = false;
#if RS2_SCENE225_RUNTIME
            world_action_pending225=false;key_event_count225=0;minimap_click_pending225=false;menu_opened_cycle225=false;loading_mapback_valid225=false;
            mouse_button225=cycle_click_button=0;mouse_press225=(ClientEvent){0};
#endif
            session.stream.buffered = true;   // out.pos = 0
            title_screen_unload(&title);
            ingame = true; live_server = true; actions.live = true;
            printf("[PASS] Connected to LIVE SERVER! (is_mod=%d)\n", session.is_mod);
        } else if (reply == LOGIN_RECONNECT_SUCCESS) {
            system_update_timer = 0;
            context_menu_clear(&menu); menu.visible = false;
            session.stream.buffered = true;   // out.pos = 0
            ingame = true; live_server = true; actions.live = true;
            printf("[STATE] reconnected\n");
        } else if (reconnect || ingame) {
            client_logout();   // tryReconnect: still not in game -> logout()
        } else {
            title_screen_set_login_reply(&title, (int)reply);
            printf("[STATE] login message0=\"%s\" message1=\"%s\"\n", title.message0, title.message1);
        }
}

void client_shutdown(void) {
    input_tracking_disable(&input_tracking);
    client_login_dns_clear225();
    printf("[Lost City 2004 C] Shutting down...\n");

    if (live_server) {
        protocol_close_session(&session);
    }
    map_streamer_clear(&streamer);
    effects_dispose(&effects);
    spotanim_free(&spot_cfg);
    zone_sync_dispose(&zones);
    entity_sync_cleanup(&sync_ctx);
    client_stream_global_cleanup();

    if (has_minimap) minimap_free(&minimap);
    free(pixels);
#if RS2_SCENE225_RUNTIME
    scene_runtime225_destroy(scene_runtime);scene_runtime=NULL;model_use_preview225(false);
#endif
    world3d_dispose(world3d);
    free(world3d);
    free(world);
    npc_entity_free(&hans);
    idk_free(&idk_cfg);
    npc_free(&npc_cfg);
    loc_free(&loc_cfg);
    flo_free(&flo_cfg);
    obj_free(&obj_cfg);
    seq_free(&seq_cfg);
    actor_models_clear();
#if RS2_PIX3D_REV225_TEXTURE
    pix3d225_texture_cache_clear();
#endif
    varp_type_free(&texture_varp_types);
    arena_free(&arena_frame);
    component_models_clear225(&comp_cfg);
    arena_free(&arena_perm);

    audio_close();
    wave_free_all();
    wordfilter_tables_free(&chat_filter);
    chatbox_free(&chatbox);
}

bool client_running(void) { return running; }
void client_stop(void) { running = false; }
uint32_t *client_pixels(void) { return pixels; }
int client_rendered_frames(void) { return rendered_frames; }
const char *client_frame_capture_path(void) { return capture_this_frame ? frame_capture_path : NULL; }

uint64_t client_frame_hash(void) {
    uint64_t hash = 14695981039346656037ULL;
    for (int i = 0; i < CLIENT_WIDTH * CLIENT_HEIGHT; i++) {
        hash ^= pixels[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void client_build_minimap(int level) {
#if RS2_SCENE225_RUNTIME
    if(!minimap_build225(&minimap,world,world3d,&media,level,&loc_cfg,collision_maps)) {
        fprintf(stderr,"Cannot build original minimap\n");running=false;
    }
#else
    minimap_build(&minimap,world,world3d,&media,level);
#endif
}

#if RS2_SCENE225_RUNTIME
/* REBUILD_NORMAL draws on the retained viewport immediately. drawGame leaves
 * it intact until PLAYER_INFO changes sceneState to 2. */
static void client_loading_message225(bool updated,bool retain_scene) {
    if(retain_scene)for(int y=0;y<VIEWPORT_H;y++)memcpy(loading_viewport225+y*VIEWPORT_W,
        pixels+(VIEWPORT_Y+y)*CLIENT_WIDTH+VIEWPORT_X,VIEWPORT_W*sizeof(*pixels));
    if(retain_scene) {
        for(int y=0;y<160;y++)memcpy(loading_mapback225+y*168,pixels+(5+y)*CLIENT_WIDTH+561,168*sizeof(*pixels));
        loading_mapback_valid225=true;
    }
    uint32_t *saved=pix2d_pixels;int width=pix2d_width,height=pix2d_height;
    int left=pix2d_clip_left,top=pix2d_clip_top,right=pix2d_clip_right,bottom=pix2d_clip_bottom;
    pix2d_bind(loading_viewport225,VIEWPORT_W,VIEWPORT_H);
    pixfont_draw_string_center(&font_p12,257,151,0,"Loading - please wait.");
    pixfont_draw_string_center(&font_p12,256,150,0xffffff,"Loading - please wait.");
    if(updated) {
        const char *message="Map area updated since last visit, so load will take longer this time only";
        pixfont_draw_string_center(&font_p12,257,166,0,message);
        pixfont_draw_string_center(&font_p12,256,165,0xffffff,message);
    }
    pix2d_bind(saved,width,height);pix2d_set_clip(left,top,right,bottom);
}
#endif

static void client_entity_movement(PathingEntity *entity) {
#if RS2_SCENE225_RUNTIME
    pathing_entity_validate225(entity,entity==&player.base);
    if(!pathing_entity_update_exact(entity,&seq_cfg,game_ticks))pathing_entity_update_movement225(entity,&seq_cfg);
#else
    pathing_entity_update_movement(entity);
#endif
}

#if RS2_SCENE225_RUNTIME
static void client_walk_result225(void) {
    if(world3d->click_tile_x<0)return;
    int x=world3d->click_tile_x,z=world3d->click_tile_z;
    world3d->click_tile_x=-1;
    PathResult route;
    if(!pathfinder_find_path(&pathfinder,&collision_maps[sync_ctx.current_plane],
        player.base.path_tile_x[0],player.base.path_tile_z[0],x,z,true,&route))return;
    bool sent=true;
    if(live_server)sent=protocol_send_route(&session,streamer.base_tile_x,streamer.base_tile_z,
        player.base.path_tile_x[0],player.base.path_tile_z[0],route.steps_x,route.steps_z,route.step_count,false);
    else pathing_entity_queue_path(&player.base,route.steps_x,route.steps_z,route.step_count,false);
    if(!sent){fprintf(stderr,"[ACTION] Route send failed\n");return;}
    if(has_minimap && route.step_count>0) {
        minimap.flag_scene_tile_x=route.steps_x[route.step_count-1];
        minimap.flag_scene_tile_z=route.steps_z[route.step_count-1];
    }
    click_feedback_start(&click_feedback,CLICK_WALK,pending_click_x,pending_click_y);
    if(trace && route.step_count>0)printf("[ACTION] route sent destination=(%d,%d)\n",
        streamer.base_tile_x+route.steps_x[route.step_count-1],streamer.base_tile_z+route.steps_z[route.step_count-1]);
}
#endif

static ActionResult client_dispatch_action(MenuAction action) {
#if RS2_SCENE225_RUNTIME
    bool world_op=action.type==MENU_NPC_OP || action.type==MENU_LOC_OP || action.type==MENU_OBJ_OP || action.type==MENU_PLAYER_OP ||
        (action.type>=MENU_ITEM_ON_NPC && action.type<=MENU_SPELL_ON_PLAYER);
    if(world_op) {
        /* GameShell records the click; handleMouseInput uses its operation
         * after network/actor updates and after the old cross has advanced. */
        world_action225=action;world_action_x225=actions.click_x;world_action_y225=actions.click_y;
        world_action_pending225=true;return ACTION_LOCAL;
    }
#endif
    return client_action_dispatch(&actions,action);
}
#if RS2_SCENE225_RUNTIME
static bool client_location_present225(void *user,uint32_t bits) {
    SceneTraversal225 *scene=user;int level=sync_ctx.current_plane;
    int x=bits&127,z=(bits>>7)&127;
    if(!scene || level<0 || level>=scene->max_level || x>=scene->grid_x || z>=scene->grid_z)return false;
    int id=scene->grid[(level*scene->grid_x+x)*scene->grid_z+z];
    if(id<0 || (size_t)id>=scene->ground_count)return false;
    SceneGround225 *g=&scene->grounds[id];
    if((g->wall.present && (uint32_t)g->wall.bitset==bits) ||
       (g->decor.present && (uint32_t)g->decor.bitset==bits) ||
       (g->ground_decor.present && (uint32_t)g->ground_decor.bitset==bits))return true;
    for(int i=0;i<g->loc_count;i++)if((uint32_t)scene->locations[g->locs[i]].bitset==bits)return true;
    return false;
}
static bool client_override_chat225(void) {
    /* Original drawWildyLevel computes these regional restrictions before
     * handleInput builds player options. */
    int x=(player.base.x>>7)+streamer.base_tile_x,z=(player.base.z>>7)+streamer.base_tile_z;
    bool restricted=(x>=3053 && x<=3156 && z>=3056 && z<=3136) ||
        (x>=3072 && x<=3118 && z>=9492 && z<=9535);
    if(restricted && x>=3139 && x<=3199 && z>=3008 && z<=3062)restricted=false;
    return restricted;
}
#endif
