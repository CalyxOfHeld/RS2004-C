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
#include "ui/component_packets.h"
#include "ui/client_actions.h"
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
#include "dash3d/effects.h"
#include "ui/combat_feedback.h"
#include "audio/wave.h"
#include "audio/midi.h"
#include "ui/chatbox.h"
#include "ui/mouse_picking.h"
#include "ui/varp.h"
#include "entity/pathing_entity.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "net/entity_sync.h"
#include "net/zone_sync.h"
#include "dash3d/location_anim.h"
#include "dash3d/location_merge.h"
#include "net/client_stream.h"
#include "net/protocol.h"
#include "net/rsa.h"

#define CLIENT_WIDTH  789
#define CLIENT_HEIGHT 532

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

#include "verification_input.h"

int main(int argc, char *argv[]) {
    const char *capture_path = NULL;
    bool offline = false;
    bool trace=false;
    const char *input_path=NULL;
    bool capture_rebuild = false;
    bool capture_clicks = false;
    int last_captured_cross = -1, last_cross_cycle = 400;
    int capture_frame = 10;
    int capture_interval = 0;
    int server_port = 43594;
    int walk_x = -1, walk_z = -1;
    int capture_yaw = 0, capture_pitch = 128;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--offline") == 0) offline = true;
        else if (strcmp(argv[i], "--trace") == 0) trace=true;
        else if (strcmp(argv[i], "--input-script") == 0 && i+1<argc) input_path=argv[++i];
        else if (strcmp(argv[i], "--input-follow") == 0) verification_follow_input=true;
        else if (strcmp(argv[i], "--rebuild") == 0) capture_rebuild = true;
        else if (strcmp(argv[i], "--capture-clicks") == 0) capture_clicks = true;
        else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) capture_path = argv[++i];
        else if (strcmp(argv[i], "--capture-frame") == 0 && i + 1 < argc) capture_frame = atoi(argv[++i]);
        else if (strcmp(argv[i], "--capture-interval") == 0 && i + 1 < argc) capture_interval = atoi(argv[++i]);
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) server_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--walk-to") == 0 && i + 2 < argc) { walk_x = atoi(argv[++i]); walk_z = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--camera-yaw") == 0 && i + 1 < argc) capture_yaw = atoi(argv[++i]) & 2047;
        else if (strcmp(argv[i], "--camera-pitch") == 0 && i + 1 < argc) capture_pitch = atoi(argv[++i]);
        else { fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]); return 1; }
    }
    if (capture_interval < 0 || ((capture_interval || capture_clicks) && !capture_path) || capture_frame < 1 || capture_pitch < 128 || capture_pitch > 383 || server_port < 1 || server_port > 65535) {
        fprintf(stderr, "Invalid capture frame, camera pitch (128..383), or server port (1..65535)\n"); return 1;
    }
    if ((capture_rebuild || walk_x >= 0 || walk_z >= 0) && (!capture_path || !offline)) {
        fprintf(stderr, "Scripted rebuild/walk requires --offline and --capture\n"); return 1;
    }
    const char *login_username = getenv("RS2_USERNAME");
    const char *login_password = getenv("RS2_PASSWORD");
    if (!offline && (!login_username || !*login_username || !login_password || !*login_password)) {
        fprintf(stderr, "Live login requires both RS2_USERNAME and RS2_PASSWORD.\n");
        fprintf(stderr, "Use play_client.bat to enter your local game account, or supply both environment variables. See MANUAL-TESTING.md.\n");
        fprintf(stderr, "No connection was attempted. --offline is only an explicit demonstration.\n");
        return 1;
    }
    setbuf(stdout, NULL);
    int rendered_frames = 0;
    Uint32 last_keepalive=SDL_GetTicks();

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

    if(input_path && (!capture_path || !verification_load_input(input_path))) {
        fprintf(stderr,"Invalid input script; --capture is required\n");return 1;
    }
    printf("[Lost City 2004 C] Starting RS2 Revision 225 Client...\n");

    Arena arena_perm = arena_create(48 * 1024 * 1024);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    audio_init();
    midi_init();

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
        pix3d_set_texture_palette(id,image->palette,image->palette_count);
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

    SpotAnimConfig spot_cfg;
    if(!spotanim_unpack(&spot_cfg,config_jag,&seq_cfg)) {fprintf(stderr,"Spot animation cache decode failed\n");return 1;}

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
    int tab_interface_id[14] = {
        2423, 3917, 638, 3213, 1644, 5608, 1151, -1, 5065, 5715, 2449, 904, 149, 962
    };
    int sticky_chat_interface_id = -1;
    int flashing_tab = -1;
    bool dev_tabs_unlocked = false;

    // Hint Arrow state (Packet 25)
    int hint_type = 0;       // 0 = none, 1 = npc, 2 = tile/coord, 10 = player
    int hint_npc = -1;
    int hint_tile_x = 0;
    int hint_tile_z = 0;
    int hint_offset_x = 0;
    int hint_offset_z = 0;
    int hint_height = 0;
    int hint_player = -1;

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
    world3d->sequences=&seq_cfg;
    world3d->trace_location_animations=trace;

    if (l_data) {
        world_load_locations(world, l_data, l_len, &loc_cfg, world3d, collision_maps, 0, 0);
        free(l_data);
    }

    world_build(world, world3d, &flo_cfg, collision_maps);
    loc_clear_model_cache(false);
    printf("[PASS] 3D Scene built successfully!\n");

    // Initialize Minimap Radar & Compass Needle
    Minimap minimap;
    bool has_minimap = false;
    if (has_media) {
        minimap_init(&minimap, &media, &arena_perm);
        minimap_build(&minimap, world, world3d, &media, 0);
        has_minimap = true;
        printf("[PASS] Minimap radar & compass needle initialized\n");
    }

    // 4. Spawn Player Character (Lumbridge Courtyard at Tile 32, 32)
    SceneEffects effects={0};effects.config=&spot_cfg;effects.trace=trace;
    ZoneSync zones={0};zones.effects=&effects;zones.trace=trace;zones.objects=&obj_cfg;zones.local_pid=-1;
    EntitySyncContext sync_ctx;
    entity_sync_init(&sync_ctx, &npc_cfg);sync_ctx.trace=trace;
    sync_ctx.seq_cfg=&seq_cfg;
    zones.entities=&sync_ctx;
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

    Arena arena_player_model=arena_create(4*1024*1024);
    Arena arena_npc_models=arena_create(16*1024*1024);
    Model **npc_models=calloc(npc_cfg.count,sizeof(Model *));
    Model *player_rest_model = player_entity_get_model(&player, &idk_cfg, &obj_cfg, NULL, &arena_player_model);
    if (player_rest_model) {
        model_create_label_references(player_rest_model, &arena_player_model);
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
    InterfaceState interfaces; interface_state_init(&interfaces);

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
    world3d->picker=&picker;
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
        if (has_minimap) minimap_build(&minimap, world, world3d, &media, 0);
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
    ClickFeedback click_feedback = {0};
    bool pending_click = false;
    int pending_click_x = 0, pending_click_y = 0;

    bool running = true;
    SDL_Event event;

    const uint8_t *keystates = SDL_GetKeyboardState(NULL);

    printf("[INFO] Entering main render loop. Controls:\n");
    printf("       [Arrow Keys] Rotate camera Pitch & Yaw\n");
    printf("       [W, A, S, D] Move only in explicit offline preview\n");
    printf("       [Left Click] Walk to tile / Interact / Select tab\n");
    printf("       [Right Click] Open available target operations\n");
    printf("       [ESC] Close active interface/menu, otherwise exit\n");

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
    if (!offline) printf("[INFO] Checking for live 2004scape server at 127.0.0.1:%d...\n", server_port);
    LoginResponse login_res = offline ? LOGIN_NET_ERROR : protocol_login(&session,"127.0.0.1",server_port,login_username,login_password,false);
    if (login_res == LOGIN_SUCCESS || login_res == LOGIN_MOD_SUCCESS) {
        live_server = true;
        player.visible=false;
        printf("[PASS] Connected to LIVE SERVER! (is_mod=%d)\n", session.is_mod);
    } else if(offline) {
        printf("[MODE] Explicit offline demonstration; inventory/entities are fixtures, no gameplay verification.\n");
    } else {
        fprintf(stderr,"Login failed (%d) at 127.0.0.1:%d.\n",login_res,server_port);
        if (login_res == LOGIN_NET_ERROR) {
            fprintf(stderr,"Start start_local_server.bat, wait for World ready, then use play_client.bat (port 43595). See MANUAL-TESTING.md.\n");
        } else if (login_res == LOGIN_ACCOUNT_LOGGED_IN) {
            fprintf(stderr,"This account is still logged in or its previous logout is pending. Close its other client, wait for the server to finish logging it out, then retry.\n");
            fprintf(stderr,"The server returned code 5; this does not indicate a wrong password. See MANUAL-TESTING.md.\n");
        } else if (login_res == LOGIN_INVALID_CREDENTIALS) {
            fprintf(stderr,"The server rejected the username or password. Check the local account details and see MANUAL-TESTING.md.\n");
        } else {
            fprintf(stderr,"The server returned login code %d. Check MANUAL-TESTING.md for its meaning.\n",login_res);
        }
        SDL_Quit();return 1;
    }

    Chatbox chatbox;
    chatbox_init(&chatbox, "Player");
    if (live_server) {
        chatbox_add_message(&chatbox, 0, NULL, "Connected to LostCity 2004 Server.");
    } else {
        chatbox_add_message(&chatbox,0,NULL,"Offline scene preview. Gameplay requires a server.");
    }
    MenuSelection selection={0};comp_cfg.selection=&selection;
    picker.selection=&selection;
    ClientDesign design={0};
    ClientActions actions={.session=&session,.components=&comp_cfg,.varps=&varp_ctx,.objects=&obj_cfg,
        .selection=&selection,.chatbox=&chatbox,.viewport_interface=&interfaces.viewport,
        .chat_interface=&interfaces.chat,.sidebar_interface=&interfaces.sidebar,.live=live_server,.trace=trace,.idkits=&idk_cfg,.design=&design,
        .entities=&sync_ctx,.scene=world3d,.locations=&loc_cfg,.pathfinder=&pathfinder,.collision=collision_maps,.map=&streamer,
        .destination_x=has_minimap ? &minimap.flag_scene_tile_x : NULL,
        .destination_z=has_minimap ? &minimap.flag_scene_tile_z : NULL,
        .click_feedback=&click_feedback,.selected_tab=&selected_tab};
    SDL_StartTextInput();

    // High-resolution 50Hz game loop timing (matching GameShell.java:150-210)
    uint64_t otim[10];
    uint64_t initial_time = SDL_GetTicks64();
    for (int i = 0; i < 10; i++) otim[i] = initial_time;
    int otim_idx = 0;
    int var4 = 256;
    int var5 = 1;
    int var6 = 0;
    int game_ticks = 0;
    int location_anim_cycle = 0;
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

        midi_is_playing();
        verification_push_input(rendered_frames);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_TEXTINPUT) {
                for (int i = 0; event.text.text[i] != '\0'; i++) {
                    chatbox_key_press(&chatbox, event.text.text[i]);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (chatbox.count_dialog_open) {
                        chatbox_close_count_dialog(&chatbox);
                    } else if (interfaces.viewport != -1 || interfaces.sidebar != -1) {
                        if (live_server) protocol_send_close_modal(&session);
                        interface_state_init(&interfaces);
                    } else if (interfaces.chat != -1) {
                        interfaces.chat = -1;
                    } else if (menu.visible) {
                        menu.visible = false;
                    } else {
                        selected_tab = 3; // Authentic RS2: Esc toggles to inventory
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
                    if (chatbox.count_dialog_open) {
                        int count = chatbox_count_value(&chatbox);
                        if (live_server) {
                                bool sent=protocol_send_resume_p_countdialog(&session, count);
                                if(trace) printf("[ACTION] quantity %s count=%d\n",sent?"sent":"failed",count);
                        }
                        chatbox_close_count_dialog(&chatbox);
                        continue;
                    }
                    const char *cmd = live_server ? chatbox_take_input(&chatbox):chatbox_submit(&chatbox);
                    if (cmd) {
                        if (live_server && cmd[0] == ':' && cmd[1] == ':') {
                            bool sent=protocol_send_command(&session,cmd+2);
                            if(trace) printf("[ACTION] command %s bytes=%zu\n",sent?"sent":"failed",strlen(cmd+2));
                        }
                        if(live_server) {
                            if(!(cmd[0]==':' && cmd[1]==':'))
                                chatbox_add_message(&chatbox,0,NULL,"Public chat is unavailable in this build; nothing was sent.");
                            continue;
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
            } else if (event.type == SDL_MOUSEWHEEL) {
                if (interfaces.viewport != -1 && mouse_x < 530 && mouse_y < 357) {
                    Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
                    if (modal_com) {
                        int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                        int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                        int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                        int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                        component_handle_mousewheel(&comp_cfg, modal_com, mouse_x, mouse_y, mx_pos, my_pos, 0, event.wheel.y);
                    }
                } else if (selected_tab >= 0 && selected_tab < 14) {
                    int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
                    if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
                    if (tab_if > 0 && tab_if != 65535) {
                        Component *tab_com = component_get(&comp_cfg, tab_if);
                        if (tab_com) {
                            component_handle_mousewheel(&comp_cfg, tab_com, mouse_x, mouse_y, 562, 231, 0, event.wheel.y);
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                comp_cfg.mouse_x = mouse_x;
                comp_cfg.mouse_y = mouse_y;
                if (comp_cfg.drag.active) {
                    if (abs(mouse_x - comp_cfg.drag.grab_x) > 5 || abs(mouse_y - comp_cfg.drag.grab_y) > 5) {
                        comp_cfg.drag.threshold = true;
                    }
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
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;

                mouse_x = mx; mouse_y = my;
                actions.click_x=mx; actions.click_y=my;
                comp_cfg.mouse_x = mouse_x; comp_cfg.mouse_y = mouse_y;
                actions.live = live_server;
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (comp_cfg.drag.active) {
                        comp_cfg.drag.active = false;
                    }
                    if (has_fonts && menu.size > 0) context_menu_open(&menu, mx, my, &font_b12);
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    // Original handleMinimapInput hit rectangle and inverse rotation.
                    // Consume this click before stale scene/context-menu actions.
                    if (!menu.visible && has_minimap && mx >= 582 && mx < 728 && my >= 14 && my < 165) {
                        if (interfaces.viewport == -1 && !comp_cfg.drag.active && live_server) {
                            int yaw=cam.render_yaw&2047;
                            MinimapClick click;
                            PathResult route;
                            if(minimap_click_target(mx,my,player.base.x,player.base.z,yaw,&click) &&
                                pathfinder_find_path(&pathfinder,&collision_maps[sync_ctx.current_plane],
                                player.base.path_tile_x[0],player.base.path_tile_z[0],click.tile_x,click.tile_z,true,&route) && route.step_count>0) {
                                if(trace) printf("[NAV] click=(%d,%d) yaw=%d source=(%d,%d) target=(%d,%d) nearest=%d steps=%d\n",
                                    mx,my,yaw,streamer.base_tile_x+player.base.path_tile_x[0],streamer.base_tile_z+player.base.path_tile_z[0],
                                    streamer.base_tile_x+click.tile_x,streamer.base_tile_z+click.tile_z,route.reached_nearest,route.step_count);
                                bool ctrl=(SDL_GetModState()&KMOD_CTRL)!=0;
                                bool sent=protocol_send_minimap_route(&session,streamer.base_tile_x,streamer.base_tile_z,
                                    player.base.path_tile_x[0],player.base.path_tile_z[0],route.steps_x,route.steps_z,route.step_count,
                                    ctrl,click.click_x,click.click_y,yaw,player.base.x,player.base.z,route.reached_nearest);
                                if(sent) {
                                    minimap.flag_scene_tile_x=route.steps_x[route.step_count-1];
                                    minimap.flag_scene_tile_z=route.steps_z[route.step_count-1];
                                } else fprintf(stderr,"[ACTION] Minimap route send failed\n");
                                if(trace) printf("[ACTION] minimap route %s destination=(%d,%d)\n",sent?"sent":"failed",
                                    streamer.base_tile_x+route.steps_x[route.step_count-1],streamer.base_tile_z+route.steps_z[route.step_count-1]);
                            }
                        }
                        continue;
                    }
                    MenuAction action = {.type = MENU_CANCEL}; bool chosen = false;
                    if (menu.visible) {
                        chosen = context_menu_pick_action(&menu, mx, my, &action);
                        if (chosen) {
                            ActionResult result = client_action_dispatch(&actions, action);
                            if (result == ACTION_WALK && interfaces.viewport == -1) {
                                pending_click = true; pending_click_x = mx; pending_click_y = my;
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
                                    continue;
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
                                        continue;
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
                            if (mx >= 8 && mx <= 108) public_chat_setting = (public_chat_setting + 1) % 4;
                            else if (mx >= 137 && mx <= 237) private_chat_setting = (private_chat_setting + 1) % 3;
                            else if (mx >= 275 && mx <= 375) trade_chat_setting = (trade_chat_setting + 1) % 3;
                            if (live_server) protocol_send_chat_setmode(&session, public_chat_setting, private_chat_setting, trade_chat_setting);
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
                                if (menu.size > 0) { action = menu.actions[menu.size - 1]; chosen = true; }
                                if (chosen) {
                                    ActionResult result = client_action_dispatch(&actions, action);
                                    if (result == ACTION_WALK && interfaces.viewport == -1) {
                                        pending_click = true; pending_click_x = mx; pending_click_y = my;
                                    } else if (result == ACTION_UNAVAILABLE) {
                                        fprintf(stderr, "[ACTION] Unsupported action type %d; no packet sent\n", action.type);
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT && comp_cfg.scroll_drag.active) {
                    comp_cfg.scroll_drag.active = false;
                    comp_cfg.scroll_drag.target = NULL;
                    comp_cfg.scroll_drag.arrow_dir = 0;
                }
                if (event.button.button == SDL_BUTTON_LEFT && comp_cfg.drag.active) {
                    int up_x = event.button.x;
                    int up_y = event.button.y;
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
                    } else {
                        if (menu.size > 0) {
                            MenuAction action = menu.actions[menu.size - 1];
                            ActionResult result = client_action_dispatch(&actions, action);
                            if (result == ACTION_WALK && interfaces.viewport == -1) {
                                pending_click = true; pending_click_x = up_x; pending_click_y = up_y;
                            }
                        }
                    }
                    comp_cfg.drag.active = false;
                }
            }
        }

        // 50 Hz Client Logic Tick Accumulator (matching GameShell.java:200-206)
        while (var6 < 256) {
            game_ticks++;
            sync_ctx.cycle=game_ticks;effects.cycle=game_ticks;
            audio_tick();
            if (comp_cfg.drag.active) comp_cfg.drag.cycles++;
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
                if(SDL_GetTicks()-last_keepalive>=1000) {
                    if(!protocol_send_keepalive(&session)) {protocol_close_session(&session);live_server=false;}
                    last_keepalive=SDL_GetTicks();
                }
                int opcode = 0;
                uint8_t payload[65536];
                size_t pkt_len = 0;
                int receive_status;
                while ((receive_status=protocol_poll_packet(&session, &opcode, payload, sizeof(payload), &pkt_len)) > 0) {
                    int zone_result=zone_sync_apply(&zones,world,world3d,&loc_cfg,collision_maps,sync_ctx.current_plane,opcode,payload,pkt_len);
                    if(zone_result<0) {fprintf(stderr,"Malformed zone packet %d\n",opcode);protocol_close_session(&session);live_server=false;break;}
                    if(zone_result>0) {
                        if(zones.dirty && has_minimap) minimap_build(&minimap,world,world3d,&media,sync_ctx.current_plane);
                        zones.dirty=false;continue;
                    }
                    int panel_update=interface_state_apply(&interfaces,opcode,payload,pkt_len);
                    if(panel_update<0) {
                        fprintf(stderr,"Malformed interface packet %d\n",opcode);
                        protocol_close_session(&session);live_server=false;break;
                    }
                    if(panel_update>0) {
                        chatbox_close_count_dialog(&chatbox);
                        comp_cfg.drag.active=false;menu.visible=false;
                        if(trace) printf("[STATE] panels main=%d side=%d chat=%d\n",interfaces.viewport,interfaces.sidebar,interfaces.chat);
                        continue;
                    }
                    int update=component_apply_packet(&comp_cfg,&varp_ctx,&arena_perm,opcode,payload,pkt_len);
                    if(update<0) {
                        fprintf(stderr,"[Protocol] Malformed component packet %d (%zu bytes)\n",opcode,pkt_len);
                        protocol_close_session(&session);live_server=false;break;
                    }
                    if(update>0) {
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
                    if (opcode == 4) { // MESSAGE_GAME
                        char msg[256];
                        size_t cpy = (pkt_len < sizeof(msg) - 1) ? pkt_len : sizeof(msg) - 1;
                        memcpy(msg, payload, cpy);
                        msg[cpy] = '\0';
                        chatbox_add_message(&chatbox, 0, NULL, msg);
                        if(trace) printf("[STATE] game_message=%s\n",msg);
                    } else if (opcode == 12) { // SYNTH_SOUND
                        if (pkt_len >= 5) {
                            int sound_id = (payload[0] << 8) | payload[1];
                            int loops = payload[2];
                            int delay = (payload[3] << 8) | payload[4];
                            audio_queue_sound(sound_id, loops > 0 ? loops : 1, delay);
                        }
                    } else if (opcode == 84) { // IF_SETTAB_ACTIVE
                        if (pkt_len >= 1) {
                            int tab = payload[0];
                            if (tab >= 0 && tab < 14) selected_tab = tab;
                        }
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
                            protocol_close_session(&session);live_server=false;break;
                        }
                        memcpy(song_name,payload,name_len);
                        song_name[name_len]='\0';
                        size_t pos=name_len+1;
                        uint32_t crc=((uint32_t)payload[pos]<<24) | ((uint32_t)payload[pos+1]<<16) |
                            ((uint32_t)payload[pos+2]<<8) | payload[pos+3];
                        uint32_t size=((uint32_t)payload[pos+4]<<24) | ((uint32_t)payload[pos+5]<<16) |
                            ((uint32_t)payload[pos+6]<<8) | payload[pos+7];
                        if ((song_name[0] && size == 0) || !midi_play(song_name,crc,size))
                            fprintf(stderr,"[MIDI] Song command rejected or playback unavailable\n");
                    } else if (opcode == 212) { // MIDI_JINGLE
                        if (pkt_len < 6) {
                            fprintf(stderr,"[Protocol] Malformed MIDI_JINGLE\n");
                            protocol_close_session(&session);live_server=false;break;
                        }
                        uint32_t uncomp_len = (uint32_t)payload[2]<<24 | (uint32_t)payload[3]<<16 |
                            (uint32_t)payload[4]<<8 | payload[5];
                        if (uncomp_len > MIDI_MAX_DECODED ||
                            !midi_play_jingle(payload+6,(int)(pkt_len-6),(int)uncomp_len,
                                (uint16_t)((uint16_t)payload[0]<<8 | payload[1])))
                            fprintf(stderr,"[MIDI] Jingle command rejected or playback unavailable\n");
                    } else if (opcode == 243) { // P_COUNTDIALOG
                        chatbox_open_count_dialog(&chatbox);
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
                        Packet pinfo = packet_wrap(payload, pkt_len);
                        if(!entity_sync_player_info(&sync_ctx,&pinfo,pkt_len)) {
                            fprintf(stderr,"[Protocol] Invalid PLAYER_INFO\n");protocol_close_session(&session);live_server=false;break;
                        }
                        if(previous_plane!=sync_ctx.current_plane && has_minimap) {
                            minimap_build(&minimap,world,world3d,&media,sync_ctx.current_plane);
                            if(trace) printf("[STATE] plane=%d minimap rebuilt\n",sync_ctx.current_plane);
                        }
                        if(player.name[0]) snprintf(chatbox.username,sizeof(chatbox.username),"%.31s",player.name);
                    } else if (opcode == 19) { // UNSET_MAP_FLAG: zero payload bytes
                        if (has_minimap) minimap.flag_scene_tile_x = 0;
                        if (trace) printf("[STATE] destination cleared by server\n");
                    } else if (opcode == 1) { // NPC_INFO
                        Packet ninfo = packet_wrap(payload, pkt_len);
                        if(!entity_sync_npc_info(&sync_ctx,&ninfo,pkt_len)) {
                            fprintf(stderr,"[Protocol] Invalid NPC_INFO\n");protocol_close_session(&session);live_server=false;break;
                        }
                    } else if (opcode == 237) { // REBUILD_NORMAL
                        int old_base_x=streamer.base_tile_x,old_base_z=streamer.base_tile_z;
                        Packet rebuild_pkt = packet_wrap(payload, pkt_len);
                        if (map_streamer_parse_rebuild_packet(&streamer, &rebuild_pkt)) {
                            entity_sync_rebase(&sync_ctx,streamer.base_tile_x-old_base_x,streamer.base_tile_z-old_base_z);
                            if (has_minimap && minimap.flag_scene_tile_x != 0) {
                                minimap.flag_scene_tile_x -= streamer.base_tile_x-old_base_x;
                                minimap.flag_scene_tile_z -= streamer.base_tile_z-old_base_z;
                                if(trace) printf("[NAV] rebased destination world=(%d,%d)\n",
                                    streamer.base_tile_x+minimap.flag_scene_tile_x,streamer.base_tile_z+minimap.flag_scene_tile_z);
                            }
                            arena_reset(&arena_npc_models);memset(npc_models,0,npc_cfg.count*sizeof(Model *));
                            const char *m_dir = "../engine/data/pack/client/maps";
                            FILE *tf = fopen("../engine/data/pack/client/maps/m50_50", "rb");
                            if (tf) { fclose(tf); } else { m_dir = "engine/data/pack/client/maps"; }
                            effects_clear(&effects);
                            zone_sync_reset(&zones);
                            GroundItem **saved_items=ground_items_detach(world3d);
                            if(!saved_items) {fprintf(stderr,"Cannot retain ground items during rebuild\n");running=false;break;}
                            bool rebuilt=map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, &loc_cfg, m_dir);
                            loc_clear_model_cache(false);
                            ground_items_restore(world3d,saved_items,streamer.base_tile_x-old_base_x,streamer.base_tile_z-old_base_z);
                            if (!rebuilt) {
                                fprintf(stderr, "Live map rebuild failed\n");
                                continue;
                            }
                            if (has_minimap) minimap_build(&minimap, world, world3d, &media, sync_ctx.current_plane);
                            printf("[VERIFY] Live REBUILD_NORMAL: %d mapsquares, base=(%d,%d)\n",
                                streamer.region_count, streamer.base_tile_x, streamer.base_tile_z);
                            cam.orbit_x = player.base.x;
                            cam.orbit_z = player.base.z;
                            chatbox_add_message(&chatbox, 0, NULL, "Loaded new map region.");
                        }
                    }
                }
                if(receive_status<0 && live_server) {
                    live_server=false;protocol_close_session(&session);
                    chatbox_add_message(&chatbox,0,NULL,"Connection lost.");
                }
            }

            // B. Camera & Player Movement Controls (client.java:6671-6684)
            bool key_left = keystates[SDL_SCANCODE_LEFT];
            bool key_right = keystates[SDL_SCANCODE_RIGHT];
            bool key_up = keystates[SDL_SCANCODE_UP];
            bool key_down = keystates[SDL_SCANCODE_DOWN];

            camera_update(&cam, player.base.x, player.base.z, sync_ctx.current_plane, world3d->heightmap, world->tile_flags,
                          key_left, key_right, key_up, key_down);
            int cur_gy = camera_get_height(sync_ctx.current_plane, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
            player.base.y = cur_gy;

            // Player keyboard movement controls (relative to camera yaw)
            int forward = 0;
            int strafe = 0;
            if (keystates[SDL_SCANCODE_W]) forward += 1;
            if (keystates[SDL_SCANCODE_S]) forward -= 1;
            if (keystates[SDL_SCANCODE_A]) strafe -= 1;
            if (keystates[SDL_SCANCODE_D]) strafe += 1;

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
            if(!pathing_entity_update_exact(&player.base,&seq_cfg,game_ticks))pathing_entity_update_movement(&player.base);
            if (has_minimap && player.base.x / 128 == minimap.flag_scene_tile_x &&
                player.base.z / 128 == minimap.flag_scene_tile_z) {
                if(trace && minimap.flag_scene_tile_x) printf("[NAV] destination reached world=(%d,%d)\n",
                    streamer.base_tile_x+minimap.flag_scene_tile_x,streamer.base_tile_z+minimap.flag_scene_tile_z);
                minimap.flag_scene_tile_x = 0;
            }
            pathing_entity_update_facing(&player.base);
            pathing_entity_update_sequences(&player.base, &seq_cfg);

            if(live_server) {
                for(int i=0;i<sync_ctx.npc_count;i++) {
                    NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];if(!npc) continue;
                    pathing_entity_update_movement(&npc->base);pathing_entity_update_facing(&npc->base);pathing_entity_update_sequences(&npc->base,&seq_cfg);
                }
                for(int i=0;i<sync_ctx.player_count;i++) {
                    PlayerEntity *other=sync_ctx.players[sync_ctx.player_indices[i]];if(!other) continue;
                    if(!pathing_entity_update_exact(&other->base,&seq_cfg,game_ticks))pathing_entity_update_movement(&other->base);
                    pathing_entity_update_facing(&other->base);pathing_entity_update_sequences(&other->base,&seq_cfg);
                }
            }
            pathing_entity_update_movement(&hans.base);
            pathing_entity_update_facing(&hans.base);
            pathing_entity_update_sequences(&hans.base, &seq_cfg);

            // D. Cross animation progression (client.java:7744-7749)
            click_feedback_tick(&click_feedback);
            midi_tick();
            actor_spotanim_tick(&player.base,&spot_cfg,game_ticks);
            for(int i=0;i<sync_ctx.npc_count;i++)if(sync_ctx.npcs[sync_ctx.npc_indices[i]])actor_spotanim_tick(&sync_ctx.npcs[sync_ctx.npc_indices[i]]->base,&spot_cfg,game_ticks);
            for(int i=0;i<sync_ctx.player_count;i++)if(sync_ctx.players[sync_ctx.player_indices[i]])actor_spotanim_tick(&sync_ctx.players[sync_ctx.player_indices[i]]->base,&spot_cfg,game_ticks);
            effects_tick(&effects,world,world3d,&sync_ctx,game_ticks);
            if(!location_merge_tick(&zones,world,world3d,&loc_cfg,collision_maps,game_ticks)) {fprintf(stderr,"Cannot update merged scenery\n");running=false;break;}
            if(zones.dirty && has_minimap) minimap_build(&minimap,world,world3d,&media,sync_ctx.current_plane);
            zones.dirty=false;

            var6 += var4;
        }
        var6 &= 0xFF;

        // Viewport camera parameters for this frame
        int camera_pitch = cam.render_pitch;
        int camera_yaw   = cam.render_yaw;
        int eye_x = cam.eye_x;
        int eye_y = cam.eye_y;
        int eye_z = cam.eye_z;
        int ground_y = camera_get_height(sync_ctx.current_plane, player.base.x, player.base.z, world3d->heightmap, world->tile_flags);
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
        int cross_frame=click_feedback_frame(&click_feedback);
        bool capture_cross=capture_clicks && cross_frame>=0 &&
            (cross_frame!=last_captured_cross || click_feedback.cycle<last_cross_cycle);
        last_captured_cross=cross_frame; last_cross_cycle=click_feedback.cycle;
        bool capture_this_frame = capture_path && (rendered_frames == capture_frame - 1 || capture_cross ||
            (capture_interval && (rendered_frames + 1) % capture_interval == 0));
        char interval_path[1024];
        const char *frame_capture_path = capture_path;
        if (capture_this_frame && rendered_frames != capture_frame - 1) {
            int length=snprintf(interval_path,sizeof(interval_path),"%s.frame-%06d.bmp",capture_path,rendered_frames+1);
            if(length<0 || (size_t)length>=sizeof(interval_path)) {fprintf(stderr,"Capture path too long\n");return 1;}
            frame_capture_path=interval_path;
        }
        location_anim_tick(world3d,&loc_cfg,game_ticks-location_anim_cycle);
        location_anim_cycle=game_ticks;
        world3d_render(world3d,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,0,sync_ctx.current_plane);
        // Optional replay helpers resolve pointers from actual received models;
        // they only enqueue SDL input and never create items or outcomes.
        if(verification_loc_id>=0)for(int x=0;x<104 && verification_loc_id>=0;x++)for(int z=0;z<104 && verification_loc_id>=0;z++) {
            GroundTile *tile=&world3d->tiles[sync_ctx.current_plane][x][z];
            for(int k=0;k<tile->loc_count && verification_loc_id>=0;k++) {
                WorldLocation *loc=&tile->locs[k];if(((loc->bitset>>14)&32767)!=verification_loc_id || !loc->model)continue;
                int min_y=0,max_y=0;
                for(int v=0;v<loc->model->num_points;v++){if(loc->model->point_y[v]<min_y)min_y=loc->model->point_y[v];if(loc->model->point_y[v]>max_y)max_y=loc->model->point_y[v];}
                int sx,sy;
                if(!project_from_point(loc->x,loc->y+(min_y+max_y)/2,loc->z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&sx,&sy))continue;
                int best=100000,bx=-1,by=-1;
                for(int dy=-30;dy<=30;dy+=2)for(int dx=-24;dx<=24;dx+=2) {
                    int px=sx+dx,py=sy+dy,score=dx*dx+dy*dy;
                    if(score>=best || px<VIEWPORT_X || px>=VIEWPORT_X+VIEWPORT_W || py<VIEWPORT_Y || py>=VIEWPORT_Y+VIEWPORT_H)continue;
                    if(model_draw_pickable(loc->model,loc->yaw,camera_pitch,camera_yaw,loc->x-eye_x,loc->y-eye_y,loc->z-eye_z,px,py)){best=score;bx=px;by=py;}
                }
                if(bx>=0) {
                    SDL_Event pointer;memset(&pointer,0,sizeof(pointer));pointer.type=SDL_MOUSEMOTION;pointer.motion.x=bx;pointer.motion.y=by;SDL_PushEvent(&pointer);
                    verification_cursor_x=bx;verification_cursor_y=by;verification_loc_id=-1;
                    printf("[INPUT] rendered loc=%d world=(%d,%d,%d) pointer=(%d,%d)\n",(loc->bitset>>14)&32767,sync_ctx.current_plane,streamer.base_tile_x+x,streamer.base_tile_z+z,bx,by);
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
                            SDL_Event pointer;memset(&pointer,0,sizeof(pointer));pointer.type=SDL_MOUSEMOTION;
                            pointer.motion.x=px;pointer.motion.y=py;SDL_PushEvent(&pointer);
                            verification_cursor_x=px;verification_cursor_y=py;verification_obj_id=-1;
                            printf("[INPUT] rendered ground item=%d world=(%d,%d) pointer=(%d,%d)\n",item->id,
                                streamer.base_tile_x+x,streamer.base_tile_z+z,px,py);
                        }
                    }
                }
            }
        if (capture_this_frame) {
            char terrain_path[2048];
            snprintf(terrain_path, sizeof(terrain_path), "%s.terrain.bmp", frame_capture_path);
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

        if (offline && hans_rest_model) {
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

        if(live_server) {
            for(int index=0;index<sync_ctx.npc_count;index++) {
                int slot=sync_ctx.npc_indices[index];NpcEntity *npc=sync_ctx.npcs[slot];
                if(!npc || !npc->has_type) continue;
                int type=npc->type.index;if(type<0 || type>=npc_cfg.count) continue;
                if(!npc_models[type]) {
                    if(arena_npc_models.capacity-arena_npc_models.offset<1024*1024) {
                        arena_reset(&arena_npc_models);memset(npc_models,0,npc_cfg.count*sizeof(Model *));
                    }
                    npc_models[type]=npc_entity_get_model(npc,&arena_npc_models);
                    if(npc_models[type]) model_create_label_references(npc_models[type],&arena_npc_models);
                }
                size_t mark=arena_frame.offset;Model *model=model_copy(npc_models[type],&arena_frame);
                if(model) {
                    if (npc->base.height == 0) {
                        int min_y = 0;
                        for (int p = 0; p < model->num_points; p++) {
                            if (model->point_y[p] < min_y) min_y = model->point_y[p];
                        }
                        if (min_y < 0) npc->base.height = -min_y;
                    }
                    model_create_label_references(model,&arena_frame);
                    int frame=current_entity_frame(&npc->base,&seq_cfg);
                    if(frame>=0) model_apply_transform(model,frame);
                    model_calculate_normals(model,64,850,-30,-50,-30);
                    model=actor_with_spotanim(model,&npc->base,&spot_cfg,&arena_frame);
                    int y=camera_get_height(sync_ctx.current_plane,npc->base.x,npc->base.z,world3d->heightmap,world->tile_flags);
                    if(verification_npc_type==type) {
                        int sx,sy;
                        if(project_from_point(npc->base.x,y-npc->base.height/2,npc->base.z,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,&sx,&sy)) {
                            int best=100000,bx=-1,by=-1;
                            for(int dy=-20;dy<=20;dy+=2) for(int dx=-16;dx<=16;dx+=2) {
                                int px=sx+dx,py=sy+dy,score=dx*dx+dy*dy;
                                if(score>=best || px<VIEWPORT_X || px>=VIEWPORT_X+VIEWPORT_W || py<VIEWPORT_Y || py>=VIEWPORT_Y+VIEWPORT_H)continue;
                                if(model_draw_pickable(model,npc->base.yaw,camera_pitch,camera_yaw,npc->base.x-eye_x,y-eye_y,npc->base.z-eye_z,px,py)){best=score;bx=px;by=py;}
                            }
                            if(bx>=0) {
                                SDL_Event pointer;memset(&pointer,0,sizeof(pointer));pointer.type=SDL_MOUSEMOTION;
                                pointer.motion.x=bx;pointer.motion.y=by;SDL_PushEvent(&pointer);
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
                    model_create_label_references(model,&arena_frame);int frame=current_entity_frame(&other->base,&seq_cfg);
                    if(frame>=0) model_apply_transform(model,frame);
                    model_calculate_normals(model,64,850,-30,-50,-30);
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

        if (!player_rest_model || player.appearance_hashcode != last_player_appearance_hash) {
            arena_reset(&arena_player_model);
            player_rest_model = player_entity_get_model(&player, &idk_cfg, &obj_cfg, NULL, &arena_player_model);
            if (player_rest_model) {
                model_create_label_references(player_rest_model, &arena_player_model);
                last_player_appearance_hash = player.appearance_hashcode;
            }
        }

        if (player.visible && player_rest_model) {
            Model *p_model = model_copy(player_rest_model, &arena_frame);
            if (p_model) {
                if (p_frame_id != -1) {
                    model_apply_transform(p_model, p_frame_id);
                }
                model_calculate_normals(p_model, 64, 850, -30, -50, -30);
                p_model=actor_with_spotanim(p_model,&player.base,&spot_cfg,&arena_frame);
                p_model=player_with_location(p_model,&player,game_ticks,ground_y,&arena_frame);
                int px = player.base.x - eye_x;
                int py = ground_y - eye_y;
                int pz = player.base.z - eye_z;
                model_draw(p_model, player.base.yaw, camera_pitch, camera_yaw, px, py, pz);
            }
        }

        effects_draw(&effects,&arena_frame,sync_ctx.current_plane,eye_x,eye_y,eye_z,camera_pitch,camera_yaw);
        pix3d_end_scene();
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
                    int h = target_npc->base.height > 0 ? target_npc->base.height : 200;
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
                    int h = target_pl->base.height > 0 ? target_pl->base.height : 200;
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

        // Dynamically build context menu & resolve primary hover action
        if (!menu.visible && !(comp_cfg.drag.active && comp_cfg.drag.threshold && comp_cfg.drag.cycles >= 5)) {
            context_menu_clear(&menu);

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
            if(live_server) mouse_picker_build_live_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&sync_ctx);
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
            int active_chat_if = interfaces.chat != -1 ? interfaces.chat : sticky_chat_interface_id;
            if (active_chat_if != -1) {
                Component *chat_com = component_get(&comp_cfg, active_chat_if);
                if (chat_com) {
                    component_build_menu(&comp_cfg, chat_com, mouse_x, mouse_y, 22, 375, 0, &menu, &obj_cfg);
                }
            }
        }

        }

        if(verification_inventory_slot>=0 && !menu.visible && selected_tab==3 && interfaces.sidebar==-1) {
            int root_id=tab_interface_id[3];Component *root=component_get(&comp_cfg,root_id);
            for(int y=233;y<491 && verification_inventory_slot>=0;y+=4)
                for(int x=564;x<751 && verification_inventory_slot>=0;x+=4) {
                    int slot=-1;Component *grid=root?component_find_inv_slot(&comp_cfg,root,x,y,562,231,0,&slot):NULL;
                    if(!grid || grid->id!=3214 || slot!=verification_inventory_slot || !grid->inv_slot_obj_id || grid->inv_slot_obj_id[slot]<=0) continue;
                    SDL_Event pointer;memset(&pointer,0,sizeof(pointer));pointer.type=SDL_MOUSEMOTION;
                    pointer.motion.x=x;pointer.motion.y=y;SDL_PushEvent(&pointer);
                    verification_cursor_x=x;verification_cursor_y=y;verification_inventory_slot=-1;
                    printf("[INPUT] inventory slot=%d item=%d pointer=(%d,%d)\n",slot,grid->inv_slot_obj_id[slot]-1,x,y);
                }
        }
        context_menu_prioritize(&menu);
        if(verification_option[0] && menu.visible) {
            for(int index=menu.size-1;index>=0;index--) {
                if(strncmp(menu.options[index],verification_option,strlen(verification_option))) continue;
                SDL_Event click;memset(&click,0,sizeof(click));click.type=SDL_MOUSEBUTTONDOWN;click.button.button=SDL_BUTTON_LEFT;
                click.button.x=menu.x+menu.width/2;click.button.y=menu.y+18+(menu.size-1-index)*15+7;
                SDL_PushEvent(&click);
                printf("[INPUT] rendered menu option=%s pointer=(%d,%d)\n",menu.options[index],click.button.x,click.button.y);
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
                snprintf(tooltip_buf, sizeof(tooltip_buf), "%s @whi@/ %d more options", top_opt, menu.size - 2);
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
            if (has_minimap) {
                minimap_draw(&minimap, &media, player.base.x, player.base.z, camera_yaw, 561, 5);
                for(int x=0;x<world3d->max_tile_x;x++) for(int z=0;z<world3d->max_tile_z;z++)
                    if(world3d->tiles[sync_ctx.current_plane][x][z].items)
                        minimap_draw_dot(player.base.x,player.base.z,x*128+64,z*128+64,camera_yaw,561,5,media.mapdots[0],media.mapback);
                if(live_server) for(int i=0;i<sync_ctx.npc_count;i++) {
                    NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];
                    minimap_draw_npc_dot(player.base.x,player.base.z,npc,camera_yaw,561,5,media.mapdots[1],media.mapback);
                }
                if(offline) minimap_draw_npc_dot(player.base.x,player.base.z,&hans,camera_yaw,561,5,media.mapdots[1],media.mapback);
                if(live_server) for(int i=0;i<sync_ctx.player_count;i++) {
                    PlayerEntity *other=sync_ctx.players[sync_ctx.player_indices[i]];
                    minimap_draw_player_dot(player.base.x,player.base.z,other,camera_yaw,561,5,media.mapdots[2],media.mapback);
                }
                minimap_draw_destination(&minimap,&media,player.base.x,player.base.z,camera_yaw,561,5);
                // Original minimap draws the local centre marker last.
                pix2d_fill_rect(561+93,5+82,3,3,0xFFFFFF);
            }
            media_draw_tabs_flashing(&media, selected_tab, dev_tabs_unlocked ? NULL : tab_interface_id, flashing_tab, game_ticks);

            // Draw interactive Chatbox or Dialogue Interface
            if (chatbox.count_dialog_open && has_fonts) {
                chatbox_draw(&chatbox,NULL,22,375,&font_p12);
            } else if (interfaces.chat != -1 && has_components) {
                Component *chat_com = component_get(&comp_cfg, interfaces.chat);
                if (chat_com) {
                    component_draw_with_varps(&comp_cfg, chat_com, 22, 375, 0, &varp_ctx);
                }
            } else if (sticky_chat_interface_id != -1 && has_components) {
                Component *tut_com = component_get(&comp_cfg, sticky_chat_interface_id);
                if (tut_com) {
                    component_draw_with_varps(&comp_cfg, tut_com, 22, 375, 0, &varp_ctx);
                }
            } else if (has_fonts) {
                chatbox_draw(&chatbox, NULL, 22, 375, &font_p12);
            }

            // Draw active Sidebar Tab Interface
            if (has_components && selected_tab >= 0 && selected_tab < 14) {
                int tab_if = interfaces.sidebar != -1 ? interfaces.sidebar : tab_interface_id[selected_tab];
                if (offline && (tab_if <= 0 || tab_if == 65535)) tab_if = DEFAULT_TAB_INTERFACE_ID[selected_tab];
                if (tab_if > 0 && tab_if != 65535) {
                    Component *tab_com = component_get(&comp_cfg, tab_if);
                    if (tab_com) {
                        component_draw_with_varps(&comp_cfg, tab_com, 562, 231, 0, &varp_ctx);
                    }
                }
            }

            // Draw Main Screen Modal Interface (Bank 5292, Shop 300, etc.) if active
            if (interfaces.viewport != -1 && has_components) {
                Component *modal_com = component_get(&comp_cfg, interfaces.viewport);
                if (modal_com) {
                    int mw = modal_com->width > 0 ? modal_com->width : VIEWPORT_W;
                    int mh = modal_com->height > 0 ? modal_com->height : VIEWPORT_H;
                    int mx_pos = VIEWPORT_X + (VIEWPORT_W - mw) / 2;
                    int my_pos = VIEWPORT_Y + (VIEWPORT_H - mh) / 2;
                    client_design_prepare(&actions,modal_com,&arena_frame,rendered_frames);
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


        // 5. Draw Right-Click Context Menu (topmost layer)
        if (has_fonts && menu.visible) {
            context_menu_draw(&menu, mouse_x, mouse_y, &font_b12);
        }

        // Update SDL texture & present
        SDL_UpdateTexture(framebuffer, NULL, pixels, CLIENT_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
        if (capture_this_frame) {
            SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, CLIENT_WIDTH, CLIENT_HEIGHT, 32, SDL_PIXELFORMAT_RGB888);
            if (!surface || SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) != 0 ||
                SDL_SaveBMP(surface, frame_capture_path) != 0) {
                fprintf(stderr, "Client capture failed: %s\n", SDL_GetError());
                return 1;
            }
            SDL_FreeSurface(surface);
            printf("[CAPTURE] SDL renderer -> %s\n", frame_capture_path);
            if (rendered_frames == capture_frame - 1) running = false;
        }
        SDL_RenderPresent(renderer);
        rendered_frames++;
    }

    printf("[Lost City 2004 C] Shutting down...\n");

    if (live_server) {
        protocol_close_session(&session);
    }
    effects_dispose(&effects);
    spotanim_free(&spot_cfg);
    zone_sync_dispose(&zones);
    entity_sync_cleanup(&sync_ctx);
    client_stream_global_cleanup();

    if (has_minimap) minimap_free(&minimap);
    free(pixels);
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
    free(npc_models);arena_free(&arena_npc_models);arena_free(&arena_player_model);
    arena_free(&arena_frame);
    arena_free(&arena_perm);

    audio_close();
    midi_close();
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

