#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <SDL2/SDL.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "core/packet.h"
#include "core/bzip2.h"
#include "core/jstring.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include "gfx/anim.h"
#include "gfx/pixfont.h"
#include "gfx/media.h"
#include "ui/component.h"
#include "ui/context_menu.h"
#include "config/flo.h"
#include "config/seq.h"
#include "config/loc.h"
#include "config/npc.h"
#include "config/obj.h"
#include "config/idk.h"
#include "dash3d/collision_map.h"
#include "dash3d/world.h"
#include "dash3d/world3d.h"
#include "dash3d/minimap.h"
#include "dash3d/pathfinder.h"
#include "dash3d/map_streamer.h"
#include "dash3d/camera.h"
#include "audio/wave.h"
#include "ui/chatbox.h"
#include "ui/mouse_picking.h"
#include "ui/varp.h"
#include "entity/pathing_entity.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "net/entity_sync.h"
#include "net/client_stream.h"
#include "net/protocol.h"
#include "net/rsa.h"

#define CLIENT_WIDTH  765
#define CLIENT_HEIGHT 503

#define VIEWPORT_X 8
#define VIEWPORT_Y 11
#define VIEWPORT_W 512
#define VIEWPORT_H 334

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

int main(int argc, char *argv[]) {
    const char *capture_path = NULL;
    bool offline = false;
    bool capture_rebuild = false;
    int capture_frame = 10;
    int server_port = 43594;
    int walk_x = -1, walk_z = -1;
    int capture_yaw = 0, capture_pitch = 128;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--offline") == 0) offline = true;
        else if (strcmp(argv[i], "--rebuild") == 0) capture_rebuild = true;
        else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) capture_path = argv[++i];
        else if (strcmp(argv[i], "--capture-frame") == 0 && i + 1 < argc) capture_frame = atoi(argv[++i]);
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) server_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--walk-to") == 0 && i + 2 < argc) { walk_x = atoi(argv[++i]); walk_z = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--camera-yaw") == 0 && i + 1 < argc) capture_yaw = atoi(argv[++i]) & 2047;
        else if (strcmp(argv[i], "--camera-pitch") == 0 && i + 1 < argc) capture_pitch = atoi(argv[++i]);
        else { fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]); return 1; }
    }
    if (capture_frame < 1 || capture_pitch < 128 || capture_pitch > 383 || server_port < 1 || server_port > 65535) {
        fprintf(stderr, "Invalid capture frame, camera pitch (128..383), or server port (1..65535)\n"); return 1;
    }
    if ((capture_rebuild || walk_x >= 0 || walk_z >= 0) && (!capture_path || !offline)) {
        fprintf(stderr, "Scripted rebuild/walk requires --offline and --capture\n"); return 1;
    }
    setbuf(stdout, NULL);
    int rendered_frames = 0;

    // Resolve cache paths consistently when launched from Explorer or either wrapper.
    char *executable_dir = SDL_GetBasePath();
    if (!executable_dir) { fprintf(stderr, "Cannot locate client directory\n"); return 1; }
    char client_dir[1024];
    snprintf(client_dir, sizeof(client_dir), "%s..", executable_dir);
    SDL_free(executable_dir);
#ifdef _WIN32
    int directory_result = _chdir(client_dir);
#else
    int directory_result = chdir(client_dir);
#endif
    if (directory_result != 0) { perror("Cannot enter client directory"); return 1; }

    printf("[Lost City 2004 C] Starting RS2 Revision 225 Client...\n");

    Arena arena_perm = arena_create(48 * 1024 * 1024);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    audio_init();

    SDL_Window *window = SDL_CreateWindow(
        "RuneScape - May 18, 2004 (C Client)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        CLIENT_WIDTH,
        CLIENT_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *framebuffer = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        CLIENT_WIDTH,
        CLIENT_HEIGHT
    );

    uint32_t *pixels = (uint32_t *)calloc(CLIENT_WIDTH * CLIENT_HEIGHT, sizeof(uint32_t));

    // 1. Initialize Pix3D Engine
    pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();

    // 2. Load Cache Archives
    printf("[INFO] Loading 2004scape cache...\n");
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena_perm);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena_perm);
    if (!config_jag) {
        fprintf(stderr, "Error: Could not locate config cache archive!\n");
        return 1;
    }

    Jagfile *textures_jag = jagfile_load_file("../engine/data/pack/client/textures", &arena_perm);
    if (!textures_jag) textures_jag = jagfile_load_file("engine/data/pack/client/textures", &arena_perm);
    int texture_count=0;
    for (int id=0;textures_jag && id<50;id++) {
        char name[8]; snprintf(name,sizeof(name),"%d",id);
        Pix8 *image=pix8_load(textures_jag,name,0,NULL);
        if(!image) continue;
        pix8_crop(image,NULL);
        uint32_t *rgb=malloc((size_t)image->width*image->height*sizeof(uint32_t));
        if(rgb) {
            for(int i=0;i<image->width*image->height;i++) {
                uint32_t col=image->palette[image->pixels[i]], out=0;
                if(image->pixels[i]) {
                    for(int shift=0;shift<=16;shift+=8) out|=(uint32_t)(pow(((col>>shift)&255)/256.0,0.8)*256.0)<<shift;
                    if(!out) out=1;
                }
                rgb[i]=out;
            }
            pix3d_set_texture(id,rgb,image->width,image->height);
            free(rgb); texture_count++;
        }
        pix8_free(image);
    }
    printf("[Textures] Loaded %d cache textures\n",texture_count);

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena_perm);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena_perm);
    if (!models_jag) {
        fprintf(stderr, "Error: Could not locate models cache archive!\n");
        return 1;
    }

    model_init_repo(models_jag, &arena_perm);

    Jagfile *sounds_jag = jagfile_load_file("../engine/data/pack/client/sounds", &arena_perm);
    if (!sounds_jag) sounds_jag = jagfile_load_file("engine/data/pack/client/sounds", &arena_perm);
    if (sounds_jag) {
        wave_unpack(sounds_jag);
        printf("[PASS] Audio synthesizer online (sounds.dat unpacked)\n");
    }

    FloConfig flo_cfg;
    flo_unpack(&flo_cfg, config_jag);

    SeqConfig seq_cfg;
    seq_unpack(&seq_cfg, config_jag);

    LocConfig loc_cfg;
    loc_unpack(&loc_cfg, config_jag);

    NpcConfig npc_cfg;
    npc_unpack(&npc_cfg, config_jag);

    ObjConfig obj_cfg;
    obj_unpack(&obj_cfg, config_jag, false);

    IdkConfig idk_cfg;
    idk_unpack(&idk_cfg, config_jag, &arena_perm);

    printf("[INFO] Cache loaded: %d flo, %d seq, %d loc, %d npc, %d obj, %d idk\n",
           flo_cfg.count, seq_cfg.count, loc_cfg.count, npc_cfg.count, obj_cfg.count, idk_cfg.count);

    bool has_anim = anim_unpack(models_jag, &arena_perm);
    if (has_anim) {
        printf("[PASS] %d Animation Bases & %d Frames loaded from models cache\n",
               g_anim_cfg.base_count, g_anim_cfg.frame_count);
    }

    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena_perm);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena_perm);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena_perm);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena_perm);

    PixFont font_p11, font_p12, font_b12, font_q8;
    bool has_fonts = false;
    if (title_jag) {
        pixfont_load(&font_p11, title_jag, "p11", &arena_perm);
        pixfont_load(&font_p12, title_jag, "p12", &arena_perm);
        pixfont_load(&font_b12, title_jag, "b12", &arena_perm);
        pixfont_load(&font_q8,  title_jag, "q8",  &arena_perm);
        has_fonts = true;
        printf("[PASS] 2004scape Bitmap Fonts loaded (p11, p12, b12, q8)\n");
    }

    Media media;
    bool has_media = false;
    if (media_jag) {
        has_media = media_load(&media, media_jag, &arena_perm);
        if (has_media) {
            printf("[PASS] 2004scape Media artwork loaded (frames, tabs, compass, icons)\n");
        }
    }

    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena_perm);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena_perm);

    ComponentConfig comp_cfg;
    bool has_components = false;
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
            if (inv_grid && inv_grid->inv_slot_obj_id) {
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

    int tab_interface_id[14] = {
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
        147,  // 12: Player controls
        962   // 13: Music player
    };

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
        free(m_data); free(l_data); return 1;
    }

    World *world = (World*)calloc(1, sizeof(World));
    world_init(world, 104, 104);

    CollisionMap collision_maps[4];
    for (int i = 0; i < 4; i++) collision_map_init(&collision_maps[i], 104, 104);

    if (m_data) {
        world_load_ground(world, m_data, m_len, 0, 0, 50 * 64, 50 * 64);
        free(m_data);
    }

    World3D *world3d = (World3D*)calloc(1, sizeof(World3D));
    world3d_init(world3d, 104, 104, 4);

    if (l_data) {
        world_load_locations(world, l_data, l_len, &loc_cfg, world3d, collision_maps, 0, 0);
        free(l_data);
    }

    world_build(world, world3d, &flo_cfg, collision_maps);
    printf("[PASS] 3D Scene built successfully!\n");

    // Initialize Minimap Radar & Compass Needle
    Minimap minimap;
    bool has_minimap = false;
    if (has_media) {
        minimap_init(&minimap, &media, &arena_perm);
        minimap_build(&minimap, world, &flo_cfg, 0);
        has_minimap = true;
        printf("[PASS] Minimap radar & compass needle initialized\n");
    }

    // 4. Spawn Player Character (Lumbridge Courtyard at Tile 32, 32)
    EntitySyncContext sync_ctx;
    entity_sync_init(&sync_ctx, &npc_cfg);
    PlayerEntity *player_ptr = &sync_ctx.local_player;
    #define player (*player_ptr)

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
    NpcEntity hans;
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

    Model *player_rest_model = player_entity_get_model(&player, &idk_cfg, &obj_cfg, NULL, &arena_perm);
    if (player_rest_model) {
        model_create_label_references(player_rest_model, &arena_perm);
    }

    Model *hans_rest_model = npc_entity_get_model(&hans, &arena_perm);
    if (hans_rest_model) {
        model_create_label_references(hans_rest_model, &arena_perm);
    }

    Arena arena_frame = arena_create(4 * 1024 * 1024);

    // 6. Camera Settings (authentic Rev 225 orbit system)
    Camera cam;
    camera_init(&cam, player.base.x, player.base.z);
    cam.orbit_yaw = capture_yaw;
    cam.orbit_pitch = capture_pitch;
    int selected_tab = 3;   // inventory tab by default
    int viewport_interface_id = -1;
    int chat_interface_id = -1;
    int public_chat_setting = 0;  // 0: On, 1: Friends, 2: Off, 3: Hide
    int private_chat_setting = 0; // 0: On, 1: Friends, 2: Off
    int trade_chat_setting = 0;   // 0: On, 1: Friends, 2: Off

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

    ContextMenu menu;
    context_menu_init(&menu);
    MousePicker picker;
    mouse_picker_init(&picker);
    Pathfinder pathfinder;
    pathfinder_init(&pathfinder);
    MapStreamer streamer;
    map_streamer_init(&streamer);
    streamer.base_tile_x = 3200;
    streamer.base_tile_z = 3200;
    if (capture_rebuild) {
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
            fprintf(stderr, "Capture map rebuild failed\n"); return 1;
        }
        if (has_minimap) minimap_build(&minimap, world, &flo_cfg, 0);
        printf("[VERIFY] Packet-237 map rebuild completed\n");
    }
    if (walk_x >= 0 && walk_z >= 0) {
        PathResult path;
        if (!pathfinder_find_path(&pathfinder, &collision_maps[0], player.base.path_tile_x[0],
                                 player.base.path_tile_z[0], walk_x, walk_z, false, &path)) {
            fprintf(stderr, "Capture walk target is unreachable\n"); return 1;
        }
        pathing_entity_queue_path(&player.base, path.steps_x, path.steps_z, path.step_count, false);
    }
    VarpContext varp_ctx;
    varp_init(&varp_ctx);
    comp_cfg.varp_ctx = &varp_ctx;
    uint64_t last_player_appearance_hash = player.appearance_hashcode;
    int mouse_x = 0, mouse_y = 0;
    int cross_x = -1, cross_y = -1;
    int cross_mode = 0;  // 1 = yellow (ground), 2 = red (interact)
    int cross_cycle = 0;
    bool pending_click = false;
    int pending_click_x = 0, pending_click_y = 0;
    int pending_click_mode = 1;

    bool running = true;
    SDL_Event event;

    const uint8_t *keystates = SDL_GetKeyboardState(NULL);

    printf("[INFO] Entering main render loop. Controls:\n");
    printf("       [Arrow Keys] Rotate camera Pitch & Yaw\n");
    printf("       [W, A, S, D] Move Player in 3D world\n");
    printf("       [Left Click] Walk to tile / Interact / Select tab\n");
    printf("       [Right Click] Open Context Menu (Walk here, Talk-to Hans, etc.)\n");
    printf("       [ESC] Exit\n");

    // Initialize Network Session (attempt connection to local 2004scape server)
    client_stream_global_init();
    GameSession session;
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

    bool live_server = false;
    printf("[INFO] Checking for live 2004scape server at 127.0.0.1:%d...\n", server_port);
    LoginResponse login_res = offline ? LOGIN_NET_ERROR : protocol_login(&session, "127.0.0.1", server_port, "antigravity", "antigravity", false);
    if (login_res == LOGIN_SUCCESS || login_res == LOGIN_MOD_SUCCESS) {
        live_server = true;
        printf("[PASS] Connected to LIVE SERVER! (is_mod=%d)\n", session.is_mod);
    } else {
        printf("[INFO] Server offline (code %d). Starting in standalone Lumbridge simulation mode.\n", login_res);
    }

    Chatbox chatbox;
    chatbox_init(&chatbox, "Player");
    if (live_server) {
        chatbox_add_message(&chatbox, 0, NULL, "Connected to LostCity 2004 Server.");
    } else {
        chatbox_add_message(&chatbox, 0, NULL, "Welcome to RuneScape.");
        chatbox_add_message(&chatbox, 2, "Hans", "Hello there adventurer!");
    }
    SDL_StartTextInput();

    // High-resolution 50Hz game loop timing (matching GameShell.java:150-210)
    uint64_t otim[10];
    uint64_t initial_time = SDL_GetTicks64();
    for (int i = 0; i < 10; i++) otim[i] = initial_time;
    int otim_idx = 0;
    int var4 = 256;
    int var5 = 1;
    int var6 = 0;
    const int deltime = 20; // 50 Hz client tick
    const int mindel = 1;

    while (running) {
        // 1. Frame pacing & sleep calculation (GameShell.java:166-199)
        int var8 = var4;
        int var9 = var5;
        var4 = 300;
        var5 = 1;
        uint64_t cur_time = SDL_GetTicks64();
        if (otim[otim_idx] == 0) {
            var4 = var8;
            var5 = var9;
        } else if (cur_time > otim[otim_idx]) {
            var4 = (int)(((uint64_t)deltime * 2560ULL) / (cur_time - otim[otim_idx]));
        }
        if (var4 < 25) var4 = 25;
        if (var4 > 256) {
            var4 = 256;
            var5 = (int)((uint64_t)deltime - (cur_time - otim[otim_idx]) / 10ULL);
        }
        otim[otim_idx] = cur_time;
        otim_idx = (otim_idx + 1) % 10;
        if (var5 > 1) {
            for (int i = 0; i < 10; i++) {
                if (otim[i] != 0) otim[i] += var5;
            }
        }
        if (var5 < mindel) var5 = mindel;
        SDL_Delay(var5);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_TEXTINPUT) {
                for (int i = 0; event.text.text[i] != '\0'; i++) {
                    chatbox_key_press(&chatbox, event.text.text[i]);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (viewport_interface_id != -1) {
                        viewport_interface_id = -1;
                    } else if (chat_interface_id != -1) {
                        chat_interface_id = -1;
                    } else if (menu.visible) {
                        menu.visible = false;
                    } else {
                        running = false;
                    }
                } else if (event.key.keysym.sym == SDLK_F1)  selected_tab = 0;
                else if (event.key.keysym.sym == SDLK_F2)  selected_tab = 1;
                else if (event.key.keysym.sym == SDLK_F3)  selected_tab = 2;
                else if (event.key.keysym.sym == SDLK_F4)  selected_tab = 3;
                else if (event.key.keysym.sym == SDLK_F5)  selected_tab = 4;
                else if (event.key.keysym.sym == SDLK_F6)  selected_tab = 5;
                else if (event.key.keysym.sym == SDLK_F7)  selected_tab = 6;
                else if (event.key.keysym.sym == SDLK_F8)  selected_tab = 8;
                else if (event.key.keysym.sym == SDLK_F9)  selected_tab = 9;
                else if (event.key.keysym.sym == SDLK_F10) selected_tab = 10;
                else if (event.key.keysym.sym == SDLK_F11) selected_tab = 11;
                else if (event.key.keysym.sym == SDLK_F12) selected_tab = 12;
                else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    chatbox_backspace(&chatbox);
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    const char *cmd = chatbox_submit(&chatbox);
                    if (cmd) {
                        if (live_server && cmd[0] == ':') {
                            protocol_send_command(&session, cmd + 2);
                        }
                        if (strncmp(cmd, "::coords", 8) == 0) {
                            char coords_msg[64];
                            snprintf(coords_msg, sizeof(coords_msg), "Coordinates: (%d, %d)", player.base.x / 128, player.base.z / 128);
                            chatbox_add_message(&chatbox, 0, NULL, coords_msg);
                        } else if (strncmp(cmd, "::bank", 6) == 0) {
                            viewport_interface_id = 5292;
                            chatbox_add_message(&chatbox, 0, NULL, "@yel@Opened Bank interface (5292).");
                        } else if (strncmp(cmd, "::shop", 6) == 0) {
                            viewport_interface_id = 300;
                            chatbox_add_message(&chatbox, 0, NULL, "@yel@Opened Shop interface (300).");
                        } else if (strncmp(cmd, "::talk", 6) == 0) {
                            chat_interface_id = 4882;
                        } else if (strncmp(cmd, "::close", 7) == 0) {
                            viewport_interface_id = -1;
                            chat_interface_id = -1;
                        } else if (strncmp(cmd, "::clear", 7) == 0) {
                            chatbox_free(&chatbox);
                            chatbox_init(&chatbox, "Player");
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                if (viewport_interface_id != -1) {
                    Component *modal_com = component_get(&comp_cfg, viewport_interface_id);
                    if (modal_com) {
                        int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                        int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                        int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                        int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                        component_handle_mousewheel(&comp_cfg, modal_com, mouse_x, mouse_y, mx_pos, my_pos, 0, event.wheel.y);
                    }
                } else if (selected_tab >= 0 && selected_tab < 14) {
                    int tab_if = tab_interface_id[selected_tab];
                    if (tab_if != -1) {
                        Component *tab_com = component_get(&comp_cfg, tab_if);
                        if (tab_com) {
                            component_handle_mousewheel(&comp_cfg, tab_com, mouse_x, mouse_y, 562, 231, 0, event.wheel.y);
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;

                if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (has_fonts && menu.size > 0) {
                        context_menu_open(&menu, mx, my, &font_b12);
                    }
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    if (menu.visible) {
                        int act = context_menu_handle_click(&menu, mx, my);
                        if (act == 1) { // Walk here
                            pending_click = true;
                            pending_click_x = mx;
                            pending_click_y = my;
                            pending_click_mode = 1;
                        } else if (act == 3000 || (act >= 3000 && act < 4000)) { // Close window
                            viewport_interface_id = -1;
                            if (live_server) protocol_send_close_modal(&session);
                        } else if (act == 6000 || (act >= 6000 && act < 7000)) { // Dialogue continue
                            int cont_id = act - 6000;
                            if (live_server) protocol_send_resume_pausebutton(&session, cont_id);
                            chat_interface_id = -1;
                        } else if (act >= 4000 && act < 5000) { // Interface button
                            int btn_id = act - 4000;
                            Component *b_com = component_get(&comp_cfg, btn_id);
                            varp_handle_component_click(&varp_ctx, b_com);
                            if (live_server) protocol_send_button(&session, btn_id);
                        } else if (act >= 1000 && act < 2000) { // Talk-to / Action
                            int npc_id = act - 1000;
                            if (live_server) protocol_send_opnpc(&session, 1, npc_id);
                            chat_interface_id = 4882;
                            pending_click = true;
                            pending_click_x = mx;
                            pending_click_y = my;
                            pending_click_mode = 2;
                        } else if (act >= 7000) { // Item action
                            int obj_id = act - 7000;
                            if (live_server) protocol_send_opheld(&session, 1, obj_id, 0, 3214);
                            char msg[128];
                            snprintf(msg, sizeof(msg), "Action executed: %s", (menu.size > 0) ? menu.options[0] : "Item");
                            chatbox_add_message(&chatbox, 0, NULL, msg);
                        } else {
                            pending_click = true;
                            pending_click_x = mx;
                            pending_click_y = my;
                            pending_click_mode = 2;
                        }
                    } else {
                        // 1. Chat filter buttons at bottom (0, 471)
                        if (mx >= 8 && mx <= 108 && my >= 490 && my <= 522) {
                            public_chat_setting = (public_chat_setting + 1) % 4;
                            if (live_server) protocol_send_chat_setmode(&session, public_chat_setting, private_chat_setting, trade_chat_setting);
                        } else if (mx >= 137 && mx <= 237 && my >= 490 && my <= 522) {
                            private_chat_setting = (private_chat_setting + 1) % 3;
                            if (live_server) protocol_send_chat_setmode(&session, public_chat_setting, private_chat_setting, trade_chat_setting);
                        } else if (mx >= 275 && mx <= 375 && my >= 490 && my <= 522) {
                            trade_chat_setting = (trade_chat_setting + 1) % 3;
                            if (live_server) protocol_send_chat_setmode(&session, public_chat_setting, private_chat_setting, trade_chat_setting);
                        } else if (mx >= 413 && mx <= 513 && my >= 490 && my <= 522) {
                            chatbox_add_message(&chatbox, 0, NULL, "@red@Report abuse system is active.");
                        }
                        // 2. Chat dialogue continue button
                        else if (chat_interface_id != -1 && mx >= 22 && mx < 501 && my >= 375 && my < 471) {
                            if (live_server) protocol_send_resume_pausebutton(&session, chat_interface_id);
                            chat_interface_id = -1;
                        }
                        // 3. Modal viewport interface
                        else if (viewport_interface_id != -1 && mx >= VIEWPORT_X && mx < VIEWPORT_X + VIEWPORT_W && my >= VIEWPORT_Y && my < VIEWPORT_Y + VIEWPORT_H) {
                            Component *modal_com = component_get(&comp_cfg, viewport_interface_id);
                            if (modal_com) {
                                int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                                int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                                int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                                int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                                component_handle_scroll(modal_com, mx, my, mx_pos + modal_com->width, my_pos, 1);
                                if (menu.size > 0) {
                                    int top_act = menu.action_ids[menu.size - 1];
                                    if (top_act >= 3000 && top_act < 4000) {
                                        viewport_interface_id = -1;
                                        if (live_server) protocol_send_close_modal(&session);
                                    } else if (top_act >= 4000 && top_act < 5000) {
                                        int b_id = top_act - 4000;
                                        Component *btn = component_get(&comp_cfg, b_id);
                                        varp_handle_component_click(&varp_ctx, btn);
                                        if (live_server) protocol_send_button(&session, b_id);
                                    }
                                }
                            }
                        }
                        // 4. Sidebar tabs (Top row Y: 194-231, Bottom row Y: 492-529)
                        else if (my >= 194 && my <= 231) {
                            if (mx >= 549 && mx <= 583 && tab_interface_id[0] != -1) selected_tab = 0;
                            else if (mx >= 579 && mx <= 609 && tab_interface_id[1] != -1) selected_tab = 1;
                            else if (mx >= 607 && mx <= 637 && tab_interface_id[2] != -1) selected_tab = 2;
                            else if (mx >= 635 && mx <= 679 && tab_interface_id[3] != -1) selected_tab = 3;
                            else if (mx >= 676 && mx <= 706 && tab_interface_id[4] != -1) selected_tab = 4;
                            else if (mx >= 704 && mx <= 734 && tab_interface_id[5] != -1) selected_tab = 5;
                            else if (mx >= 732 && mx <= 766 && tab_interface_id[6] != -1) selected_tab = 6;
                        } else if (my >= 492 && my <= 529) {
                            if (mx >= 550 && mx <= 584 && tab_interface_id[7] != -1) selected_tab = 7;
                            else if (mx >= 582 && mx <= 612 && tab_interface_id[8] != -1) selected_tab = 8;
                            else if (mx >= 609 && mx <= 639 && tab_interface_id[9] != -1) selected_tab = 9;
                            else if (mx >= 637 && mx <= 681 && tab_interface_id[10] != -1) selected_tab = 10;
                            else if (mx >= 679 && mx <= 709 && tab_interface_id[11] != -1) selected_tab = 11;
                            else if (mx >= 706 && mx <= 736 && tab_interface_id[12] != -1) selected_tab = 12;
                            else if (mx >= 734 && mx <= 768 && tab_interface_id[13] != -1) selected_tab = 13;
                        }
                        // 5. Sidebar interface scrollbar & items
                        else if (mx >= 562 && mx < 752 && my >= 231 && my < 492) {
                            if (selected_tab >= 0 && selected_tab < 14 && tab_interface_id[selected_tab] != -1) {
                                Component *tab_com = component_get(&comp_cfg, tab_interface_id[selected_tab]);
                                if (tab_com) {
                                    component_handle_scroll(tab_com, mx, my, 562 + tab_com->width, 231, 1);
                                    if (menu.size > 0) {
                                        int top_act = menu.action_ids[menu.size - 1];
                                        if (top_act >= 4000 && top_act < 5000) {
                                            int b_id = top_act - 4000;
                                            Component *btn = component_get(&comp_cfg, b_id);
                                            varp_handle_component_click(&varp_ctx, btn);
                                            if (live_server) protocol_send_button(&session, b_id);
                                        }
                                    }
                                }
                            }
                        }
                        // 6. Viewport 3D clicking (only when modal interface not open)
                        else if (viewport_interface_id == -1 && mx >= VIEWPORT_X && mx < VIEWPORT_X + VIEWPORT_W && my >= VIEWPORT_Y && my < VIEWPORT_Y + VIEWPORT_H) {
                            if (menu.size > 0 && strstr(menu.options[menu.size - 1], "Talk-to")) {
                                int top_act = menu.action_ids[menu.size - 1];
                                if (top_act >= 1000 && top_act < 2000) {
                                    int npc_id = top_act - 1000;
                                    if (live_server) protocol_send_opnpc(&session, 1, npc_id);
                                }
                                chat_interface_id = 4882;
                                pending_click = true;
                                pending_click_x = mx;
                                pending_click_y = my;
                                pending_click_mode = 2;
                            } else {
                                pending_click = true;
                                pending_click_x = mx;
                                pending_click_y = my;
                                pending_click_mode = (picker.picked_count > 0) ? 2 : 1;
                            }
                        }
                    }
                }
            }
        }

        // 50 Hz Client Logic Tick Accumulator (matching GameShell.java:200-206)
        while (var6 < 256) {
            // A. Poll incoming network packets from live server (client.java:7665)
            if (live_server) {
                int opcode = 0;
                uint8_t payload[8192];
                size_t pkt_len = 0;
                while (protocol_poll_packet(&session, &opcode, payload, &pkt_len) > 0) {
                    if (opcode == 4) { // MESSAGE_GAME
                        char msg[256];
                        size_t cpy = (pkt_len < sizeof(msg) - 1) ? pkt_len : sizeof(msg) - 1;
                        memcpy(msg, payload, cpy);
                        msg[cpy] = '\0';
                        chatbox_add_message(&chatbox, 0, NULL, msg);
                    } else if (opcode == 12) { // SYNTH_SOUND
                        if (pkt_len >= 5) {
                            int sound_id = (payload[0] << 8) | payload[1];
                            int loops = payload[2];
                            audio_play(sound_id, loops > 0 ? loops : 1);
                        }
                    } else if (opcode == 14) { // IF_OPENCHAT
                        if (pkt_len >= 2) {
                            chat_interface_id = (payload[0] << 8) | payload[1];
                        }
                    } else if (opcode == 28) { // IF_OPENMAIN_SIDE
                        if (pkt_len >= 4) {
                            viewport_interface_id = (payload[0] << 8) | payload[1];
                            int side_com = (payload[2] << 8) | payload[3];
                            if (selected_tab >= 0 && selected_tab < 14) {
                                tab_interface_id[selected_tab] = side_com;
                            }
                        }
                    } else if (opcode == 84) { // IF_SETTAB_ACTIVE
                        if (pkt_len >= 1) {
                            int tab = payload[0];
                            if (tab >= 0 && tab < 14) selected_tab = tab;
                        }
                    } else if (opcode == 129) { // IF_CLOSE
                        viewport_interface_id = -1;
                        chat_interface_id = -1;
                    } else if (opcode == 142) { // LOGOUT
                        live_server = false;
                        protocol_close_session(&session);
                        chatbox_add_message(&chatbox, 0, NULL, "You have logged out.");
                    } else if (opcode == 146) { // IF_SETANIM
                        if (pkt_len >= 4) {
                            int com_id = (payload[0] << 8) | payload[1];
                            int seq_id = (payload[2] << 8) | payload[3];
                            Component *c = component_get(&comp_cfg, com_id);
                            if (c) c->anim_id = seq_id;
                        }
                    } else if (opcode == 150) { // VARP_SMALL
                        if (pkt_len >= 3) {
                            int varp_id = (payload[0] << 8) | payload[1];
                            int val = (int8_t)payload[2];
                            varp_set(&varp_ctx, varp_id, val);
                        }
                    } else if (opcode == 167) { // IF_SETTAB
                        if (pkt_len >= 3) {
                            int com_id = (payload[0] << 8) | payload[1];
                            int tab = payload[2];
                            if (tab >= 0 && tab < 14) tab_interface_id[tab] = com_id;
                        }
                    } else if (opcode == 168) { // IF_OPENMAIN
                        if (pkt_len >= 2) {
                            viewport_interface_id = (payload[0] << 8) | payload[1];
                        }
                    } else if (opcode == 175) { // VARP_LARGE
                        if (pkt_len >= 6) {
                            int varp_id = (payload[0] << 8) | payload[1];
                            int val = (payload[2] << 24) | (payload[3] << 16) | (payload[4] << 8) | payload[5];
                            varp_set(&varp_ctx, varp_id, val);
                        }
                    } else if (opcode == 193) { // RESET_CLIENT_VARCACHE
                        memset(varp_ctx.varps, 0, sizeof(varp_ctx.varps));
                    } else if (opcode == 195) { // IF_OPENSIDE
                        if (pkt_len >= 2) {
                            int com_id = (payload[0] << 8) | payload[1];
                            if (selected_tab >= 0 && selected_tab < 14) {
                                tab_interface_id[selected_tab] = com_id;
                            }
                        }
                    } else if (opcode == 201) { // IF_SETTEXT
                        if (pkt_len >= 2) {
                            int com_id = (payload[0] << 8) | payload[1];
                            Component *c = component_get(&comp_cfg, com_id);
                            if (c && pkt_len > 2) {
                                char text_buf[256];
                                size_t t_len = pkt_len - 2;
                                if (t_len >= sizeof(text_buf)) t_len = sizeof(text_buf) - 1;
                                memcpy(text_buf, &payload[2], t_len);
                                text_buf[t_len] = '\0';
                                c->text = arena_strdup(&arena_perm, text_buf);
                            }
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
                    } else if (opcode == 32) { // CHAT_FILTER_SETTINGS
                        if (pkt_len >= 3) {
                            public_chat_setting = payload[0];
                            private_chat_setting = payload[1];
                            trade_chat_setting = payload[2];
                        }
                    } else if (opcode == 44) { // UPDATE_STAT
                        if (pkt_len >= 6) {
                            int stat = payload[0];
                            int exp = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];
                            int lvl = payload[5];
                            if (stat >= 0 && stat < 25) {
                                varp_ctx.skill_levels[stat] = lvl;
                                varp_ctx.skill_exp[stat] = exp * 10;
                            }
                        }
                    } else if (opcode == 98) { // UPDATE_INV_FULL
                        if (pkt_len >= 3) {
                            int com_id = (payload[0] << 8) | payload[1];
                            int count = payload[2];
                            Component *c = component_get(&comp_cfg, com_id);
                            if (c) {
                                int cap = c->width * c->height;
                                if (cap <= 0) cap = 32;
                                if (!c->inv_slot_obj_id) {
                                    c->inv_slot_obj_id = arena_alloc_zero(&arena_perm, cap * sizeof(int));
                                    c->inv_slot_obj_count = arena_alloc_zero(&arena_perm, cap * sizeof(int));
                                }
                                size_t idx = 3;
                                for (int s = 0; s < count && s < cap && idx + 2 <= pkt_len; s++) {
                                    int id = (payload[idx] << 8) | payload[idx + 1];
                                    idx += 2;
                                    int amt = 0;
                                    if (idx < pkt_len) {
                                        if (payload[idx] == 255) {
                                            idx++;
                                            if (idx + 4 <= pkt_len) {
                                                amt = (payload[idx] << 24) | (payload[idx+1] << 16) | (payload[idx+2] << 8) | payload[idx+3];
                                                idx += 4;
                                            }
                                        } else {
                                            amt = payload[idx++];
                                        }
                                    }
                                    c->inv_slot_obj_id[s] = id;
                                    c->inv_slot_obj_count[s] = amt;
                                }
                            }
                        }
                    } else if (opcode == 213) { // UPDATE_INV_PARTIAL
                        if (pkt_len >= 2) {
                            int com_id = (payload[0] << 8) | payload[1];
                            Component *c = component_get(&comp_cfg, com_id);
                            if (c) {
                                int cap = c->width * c->height;
                                if (cap <= 0) cap = 32;
                                if (!c->inv_slot_obj_id) {
                                    c->inv_slot_obj_id = arena_alloc_zero(&arena_perm, cap * sizeof(int));
                                    c->inv_slot_obj_count = arena_alloc_zero(&arena_perm, cap * sizeof(int));
                                }
                                size_t idx = 2;
                                while (idx + 3 <= pkt_len) {
                                    int slot = payload[idx++];
                                    int id = (payload[idx] << 8) | payload[idx + 1];
                                    idx += 2;
                                    int amt = 0;
                                    if (idx < pkt_len) {
                                        if (payload[idx] == 255) {
                                            idx++;
                                            if (idx + 4 <= pkt_len) {
                                                amt = (payload[idx] << 24) | (payload[idx+1] << 16) | (payload[idx+2] << 8) | payload[idx+3];
                                                idx += 4;
                                            }
                                        } else {
                                            amt = payload[idx++];
                                        }
                                    }
                                    if (slot >= 0 && slot < cap) {
                                        c->inv_slot_obj_id[slot] = id;
                                        c->inv_slot_obj_count[slot] = amt;
                                    }
                                }
                            }
                        }
                    } else if (opcode == 184) { // PLAYER_INFO
                        Packet pinfo = packet_wrap(payload, pkt_len);
                        entity_sync_player_info(&sync_ctx, &pinfo, pkt_len);
                    } else if (opcode == 134) { // NPC_INFO
                        Packet ninfo = packet_wrap(payload, pkt_len);
                        entity_sync_npc_info(&sync_ctx, &ninfo, pkt_len);
                    } else if (opcode == 237) { // REBUILD_NORMAL
                        Packet rebuild_pkt = packet_wrap(payload, pkt_len);
                        if (map_streamer_parse_rebuild_packet(&streamer, &rebuild_pkt)) {
                            const char *m_dir = "../engine/data/pack/client/maps";
                            FILE *tf = fopen("../engine/data/pack/client/maps/m50_50", "rb");
                            if (tf) { fclose(tf); } else { m_dir = "engine/data/pack/client/maps"; }
                            if (!map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, &loc_cfg, m_dir)) {
                                fprintf(stderr, "Live map rebuild failed\n");
                                continue;
                            }
                            if (has_minimap) minimap_build(&minimap, world, &flo_cfg, sync_ctx.current_plane);
                            printf("[VERIFY] Live REBUILD_NORMAL: %d mapsquares, base=(%d,%d)\n",
                                streamer.region_count, streamer.base_tile_x, streamer.base_tile_z);
                            player.base.x = (streamer.center_zone_x * 8 - streamer.base_tile_x) * 128 + 64;
                            player.base.z = (streamer.center_zone_z * 8 - streamer.base_tile_z) * 128 + 64;
                            player.base.path_tile_x[0] = player.base.x / 128;
                            player.base.path_tile_z[0] = player.base.z / 128;
                            cam.orbit_x = player.base.x;
                            cam.orbit_z = player.base.z;
                            chatbox_add_message(&chatbox, 0, NULL, "Loaded new map region.");
                        }
                    }
                }
            }

            // B. Camera & Player Movement Controls (client.java:6671-6684)
            bool key_left = keystates[SDL_SCANCODE_LEFT];
            bool key_right = keystates[SDL_SCANCODE_RIGHT];
            bool key_up = keystates[SDL_SCANCODE_UP];
            bool key_down = keystates[SDL_SCANCODE_DOWN];

            camera_update(&cam, player.base.x, player.base.z, 0, world3d->heightmap, world->tile_flags,
                          key_left, key_right, key_up, key_down);
            int cur_gy = camera_get_height(0, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
            player.base.y = cur_gy;

            // Player keyboard movement controls (relative to camera yaw)
            int forward = 0;
            int strafe = 0;
            if (keystates[SDL_SCANCODE_W]) forward += 1;
            if (keystates[SDL_SCANCODE_S]) forward -= 1;
            if (keystates[SDL_SCANCODE_A]) strafe -= 1;
            if (keystates[SDL_SCANCODE_D]) strafe += 1;

            if ((forward != 0 || strafe != 0) && player.base.path_length == 0) {
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
                        if (live_server && res.step_count > 1) {
                            int8_t rel_x[100];
                            int8_t rel_z[100];
                            int count = res.step_count - 1;
                            if (count > 99) count = 99;
                            for (int i = 0; i < count; i++) {
                                rel_x[i] = (int8_t)(res.steps_x[i + 1] - res.steps_x[i]);
                                rel_z[i] = (int8_t)(res.steps_z[i + 1] - res.steps_z[i]);
                            }
                            protocol_send_walk(&session, res.steps_x[0], res.steps_z[0], count, rel_x, rel_z, false);
                        }
                    }
                }
            }

            // C. Entity Logic Updates (authentic 50 Hz stride & frame progression)
            pathing_entity_update_movement(&player.base);
            pathing_entity_update_facing(&player.base);
            pathing_entity_update_sequences(&player.base, &seq_cfg);

            pathing_entity_update_movement(&hans.base);
            pathing_entity_update_facing(&hans.base);
            pathing_entity_update_sequences(&hans.base, &seq_cfg);

            // D. Cross animation progression (client.java:7744-7749)
            if (cross_mode != 0) {
                cross_cycle += 20;
                if (cross_cycle >= 400) {
                    cross_mode = 0;
                }
            }

            var6 += var4;
        }
        var6 &= 0xFF;

        // Viewport camera parameters for this frame
        int camera_pitch = cam.render_pitch;
        int camera_yaw   = cam.render_yaw;
        int eye_x = cam.eye_x;
        int eye_y = cam.eye_y;
        int eye_z = cam.eye_z;
        int ground_y = camera_get_height(0, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
        player.base.y = ground_y;

        // Clear Framebuffer
        pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);
        pix2d_cls();

        // 1. Render 3D Viewport (0..512, 0..334)
        pix2d_set_clip(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y + VIEWPORT_H);
        pix3d_center_x = VIEWPORT_X + VIEWPORT_W / 2;
        pix3d_center_y = VIEWPORT_Y + VIEWPORT_H / 2;
        mouse_picker_reset(&picker, mouse_x, mouse_y);
        if (pending_click) {
            world3d_click(world3d, pending_click_x, pending_click_y);
        }
        world3d_render(world3d, eye_x, eye_y, eye_z, camera_pitch, camera_yaw, 0, 0);
        if (capture_path && rendered_frames == capture_frame - 1) {
            char terrain_path[1024];
            snprintf(terrain_path, sizeof(terrain_path), "%s.terrain.bmp", capture_path);
            SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels, CLIENT_WIDTH, CLIENT_HEIGHT,
                32, CLIENT_WIDTH * sizeof(uint32_t), SDL_PIXELFORMAT_RGB888);
            if (!surface || SDL_SaveBMP(surface, terrain_path) != 0) {
                fprintf(stderr, "Terrain capture failed: %s\n", SDL_GetError());
                return 1;
            }
            SDL_FreeSurface(surface);
            printf("[CAPTURE] camera=(%d,%d,%d) pitch=%d yaw=%d -> %s\n", eye_x, eye_y, eye_z,
                camera_pitch, camera_yaw, terrain_path);
            printf("[VERIFY] player tile=(%d,%d) sequence=%d frame=%d\n", player.base.x / 128,
                player.base.z / 128, player.base.secondary_seq_id, player.base.secondary_seq_frame);
        }

        if (pending_click) {
            if (world3d->click_tile_x != -1) {
                PathResult res;
                if (pathfinder_find_path(&pathfinder, &collision_maps[0],
                                         player.base.path_tile_x[0], player.base.path_tile_z[0],
                                         world3d->click_tile_x, world3d->click_tile_z, true, &res)) {
                    pathing_entity_queue_path(&player.base, res.steps_x, res.steps_z, res.step_count, false);
                    if (live_server && res.step_count > 1) {
                        int8_t rel_x[100];
                        int8_t rel_z[100];
                        int count = res.step_count - 1;
                        if (count > 99) count = 99;
                        for (int i = 0; i < count; i++) {
                            rel_x[i] = (int8_t)(res.steps_x[i + 1] - res.steps_x[i]);
                            rel_z[i] = (int8_t)(res.steps_z[i + 1] - res.steps_z[i]);
                        }
                        protocol_send_walk(&session, res.steps_x[0], res.steps_z[0], count, rel_x, rel_z, false);
                    }
                    cross_x = pending_click_x;
                    cross_y = pending_click_y;
                    cross_mode = pending_click_mode;
                    cross_cycle = 0;
                }
            }
            world3d->taking_input = false;
            pending_click = false;
        }

        // Render Entities inside viewport with full camera rotation
        arena_reset(&arena_frame);

        // Advance & render NPC Hans (idle stand sequence 808)
        int h_frame_id = -1;
        if (has_anim && hans.base.secondary_seq_id >= 0 && hans.base.secondary_seq_id < seq_cfg.count) {
            SeqType *h_seq = &seq_cfg.instances[hans.base.secondary_seq_id];
            if (h_seq->frame_count > 0 && hans.base.secondary_seq_frame < h_seq->frame_count) {
                h_frame_id = h_seq->frames[hans.base.secondary_seq_frame];
            }
        }

        if (hans_rest_model) {
            Model *h_model = model_copy(hans_rest_model, &arena_frame);
            if (h_model) {
                if (h_frame_id != -1) {
                    model_apply_transform(h_model, h_frame_id);
                }
                model_calculate_normals(h_model, 64, 768, -50, -10, -50);
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

        // Render Player (authentic walk/stand animation progression)
        int p_seq_id = player.base.secondary_seq_id;
        if (p_seq_id <= 0) p_seq_id = 808;

        int p_frame_id = -1;
        if (has_anim && p_seq_id < seq_cfg.count) {
            SeqType *p_seq = &seq_cfg.instances[p_seq_id];
            if (p_seq->frame_count > 0 && player.base.secondary_seq_frame < p_seq->frame_count) {
                p_frame_id = p_seq->frames[player.base.secondary_seq_frame];
            }
        }

        if (!player_rest_model || player.appearance_hashcode != last_player_appearance_hash) {
            player_rest_model = player_entity_get_model(&player, &idk_cfg, &obj_cfg, NULL, &arena_perm);
            if (player_rest_model) {
                model_create_label_references(player_rest_model, &arena_perm);
                last_player_appearance_hash = player.appearance_hashcode;
            }
        }

        if (player_rest_model) {
            Model *p_model = model_copy(player_rest_model, &arena_frame);
            if (p_model) {
                if (p_frame_id != -1) {
                    model_apply_transform(p_model, p_frame_id);
                }
                model_calculate_normals(p_model, 64, 768, -50, -10, -50);
                int px = player.base.x - eye_x;
                int py = ground_y - eye_y;
                int pz = player.base.z - eye_z;
                model_draw(p_model, player.base.yaw, camera_pitch, camera_yaw, px, py, pz);
            }
        }

        // Dynamically build context menu & resolve primary hover action
        context_menu_clear(&menu);

        if (viewport_interface_id != -1) {
            Component *modal_com = component_get(&comp_cfg, viewport_interface_id);
            if (modal_com) {
                int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                component_build_menu(&comp_cfg, modal_com, mouse_x, mouse_y, mx_pos, my_pos, 0, &menu, &obj_cfg);
            }
        } else if (mouse_x >= VIEWPORT_X && mouse_x < VIEWPORT_X + VIEWPORT_W && mouse_y >= VIEWPORT_Y && mouse_y < VIEWPORT_Y + VIEWPORT_H) {
            mouse_picker_build_menu(&picker, &menu, &npc_cfg, &loc_cfg, &obj_cfg, &hans, 1, &player, 1, player.combat_level);
        } else if (mouse_x >= 562 && mouse_x < 752 && mouse_y >= 231 && mouse_y < 492) {
            if (has_components && selected_tab >= 0 && selected_tab < 14) {
                int tab_if = tab_interface_id[selected_tab];
                if (tab_if != -1) {
                    Component *tab_com = component_get(&comp_cfg, tab_if);
                    if (tab_com) {
                        component_build_menu(&comp_cfg, tab_com, mouse_x, mouse_y, 562, 231, 0, &menu, &obj_cfg);
                    }
                }
            }
        } else if (chat_interface_id != -1 && mouse_x >= 22 && mouse_x < 501 && mouse_y >= 375 && mouse_y < 471) {
            Component *chat_com = component_get(&comp_cfg, chat_interface_id);
            if (chat_com) {
                component_build_menu(&comp_cfg, chat_com, mouse_x, mouse_y, 22, 375, 0, &menu, &obj_cfg);
            }
        }

        static char tooltip_buf[128];
        const char *primary_action = "";
        if (menu.size > 0) {
            const char *top_opt = menu.options[menu.size - 1];
            if (menu.size > 2) {
                snprintf(tooltip_buf, sizeof(tooltip_buf), "%s @whi@/ %d more options", top_opt, menu.size - 2);
                primary_action = tooltip_buf;
            } else {
                primary_action = top_opt;
            }
        }

        pix3d_end_scene();
        // Reset clip to full window
        pix2d_reset_clip();
        pix3d_center_x = CLIENT_WIDTH / 2;
        pix3d_center_y = CLIENT_HEIGHT / 2;

        // 2. Draw authentic 2004scape UI Shell & Frame Artwork
        if (has_media) {
            media_draw_shell(&media);
            if (has_minimap) {
                minimap_draw(&minimap, &media, player.base.x, player.base.z, camera_yaw, 561, 5);
                minimap_draw_dot(player.base.x, player.base.z, hans.base.x, hans.base.z, camera_yaw, 561, 5, media.mapdots[1]);
            }
            media_draw_tabs(&media, selected_tab);

            // Draw interactive Chatbox or Dialogue Interface
            if (chat_interface_id != -1 && has_components) {
                Component *chat_com = component_get(&comp_cfg, chat_interface_id);
                if (chat_com) {
                    component_draw_with_varps(&comp_cfg, chat_com, 22, 375, 0, &varp_ctx);
                }
            } else if (has_fonts) {
                chatbox_draw(&chatbox, NULL, 22, 375, &font_p12);
            }

            // Draw active Sidebar Tab Interface
            if (has_components && selected_tab >= 0 && selected_tab < 14) {
                int tab_if = tab_interface_id[selected_tab];
                if (tab_if != -1) {
                    Component *tab_com = component_get(&comp_cfg, tab_if);
                    if (tab_com) {
                        component_draw_with_varps(&comp_cfg, tab_com, 562, 231, 0, &varp_ctx);
                    }
                }
            }

            // Draw Main Screen Modal Interface (Bank 5292, Shop 300, etc.) if active
            if (viewport_interface_id != -1 && has_components) {
                Component *modal_com = component_get(&comp_cfg, viewport_interface_id);
                if (modal_com) {
                    int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                    int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                    int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                    int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                    component_draw_with_varps(&comp_cfg, modal_com, mx_pos, my_pos, 0, &varp_ctx);
                }
            }
        } else {
            // Fallback border
            pix2d_draw_rect(VIEWPORT_X - 1, VIEWPORT_Y - 1, VIEWPORT_W + 2, VIEWPORT_H + 2, 0x473B2E);
        }

        // 3. Draw Authentic RS2 UI Text using Bitmap Fonts
        if (has_fonts) {
            // Viewport hover text (top-left)
            if (primary_action && primary_action[0] != '\0') {
                pixfont_draw_string_taggable(&font_b12, 12, 24, 0xFFFFFF, primary_action, true);
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

        // 4. Draw Movement Cross Animation
        if (has_media && cross_mode != 0) {
            int f = cross_cycle / 100;
            if (cross_mode == 2) f += 4;
            if (f >= 0 && f < 8 && media.crosses[f]) {
                pix32_draw(media.crosses[f], cross_x - 8, cross_y - 8);
            }
        }

        // 5. Draw Right-Click Context Menu (topmost layer)
        if (has_fonts && menu.visible) {
            context_menu_draw(&menu, mouse_x, mouse_y, &font_b12);
        }

        // Update SDL texture & present
        SDL_UpdateTexture(framebuffer, NULL, pixels, CLIENT_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
        if (capture_path && rendered_frames == capture_frame - 1) {
            SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, CLIENT_WIDTH, CLIENT_HEIGHT, 32, SDL_PIXELFORMAT_RGB888);
            if (!surface || SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) != 0 ||
                SDL_SaveBMP(surface, capture_path) != 0) {
                fprintf(stderr, "Client capture failed: %s\n", SDL_GetError());
                return 1;
            }
            SDL_FreeSurface(surface);
            printf("[CAPTURE] SDL renderer -> %s\n", capture_path);
            running = false;
        }
        SDL_RenderPresent(renderer);
        rendered_frames++;
    }

    printf("[Lost City 2004 C] Shutting down...\n");

    if (live_server) {
        protocol_close_session(&session);
    }
    client_stream_global_cleanup();

    if (has_minimap) minimap_free(&minimap);
    free(pixels);
    free(world3d);
    free(world);
    npc_entity_free(&hans);
    idk_free(&idk_cfg);
    npc_free(&npc_cfg);
    loc_free(&loc_cfg);
    flo_free(&flo_cfg);
    obj_free(&obj_cfg);
    seq_free(&seq_cfg);
    arena_free(&arena_frame);
    arena_free(&arena_perm);

    audio_close();
    wave_free_all();
    SDL_StopTextInput();
    chatbox_free(&chatbox);
    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("[Lost City 2004 C] Shutdown complete.\n");
    return 0;
}

