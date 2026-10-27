#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <SDL2/SDL.h>

#include "client/client.h"
#include "client/client_log.h"
#include "client/input_tracking_sdl.h"
#ifdef _WIN32
#include "net/client_stream.h"
#include <windows.h>
#endif
#include "core/session_log.h"
#include "audio/wave.h"
#include "audio/midi.h"

static SDL_Renderer *recovery_renderer;
static SDL_Texture *recovery_texture;
static void sdl_present_recovery(const uint32_t *pixels, int width, int height) {
    (void)height;
    SDL_UpdateTexture(recovery_texture, NULL, pixels, width * sizeof(uint32_t));
    SDL_RenderClear(recovery_renderer);
    SDL_RenderCopy(recovery_renderer, recovery_texture, NULL, NULL);
    SDL_RenderPresent(recovery_renderer);
}

// SDL2 platform layer: window, GameShell 50 Hz pacing, input translation and
// presentation. The client itself lives in src/client/client.c.

static uint64_t sdl_ticks(void) { return SDL_GetTicks64(); }
// Client clock reads go into the session log when recording.
static uint64_t sdl_now_ms(void) { return client_log_time(sdl_ticks); }
static uint64_t tracking_wall_ms(void){
#ifdef _WIN32
    FILETIME time;GetSystemTimeAsFileTime(&time);uint64_t ticks=(uint64_t)time.dwHighDateTime<<32|time.dwLowDateTime;return ticks/10000-11644473600000ULL;
#else
    return SDL_GetTicks64();
#endif
}
static uint64_t sdl_tracking_now_ms(void){return client_log_time(tracking_wall_ms);}

static bool sdl_save_bmp(const char *path, const uint32_t *pixels, int width, int height) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom((void *)pixels, width, height,
        32, width * (int)sizeof(uint32_t), SDL_PIXELFORMAT_RGB888);
    bool saved = surface && SDL_SaveBMP(surface, path) == 0;
    if (!saved) fprintf(stderr, "SDL_SaveBMP: %s\n", SDL_GetError());
    if (surface) SDL_FreeSurface(surface);
    return saved;
}

static int sdl_client_key(SDL_Keycode key) {
    switch (key) {
    case SDLK_ESCAPE: return CLIENT_KEY_ESCAPE;
    case SDLK_F1: return CLIENT_KEY_F1;   case SDLK_F2: return CLIENT_KEY_F2;
    case SDLK_F3: return CLIENT_KEY_F3;   case SDLK_F4: return CLIENT_KEY_F4;
    case SDLK_F5: return CLIENT_KEY_F5;   case SDLK_F6: return CLIENT_KEY_F6;
    case SDLK_F7: return CLIENT_KEY_F7;   case SDLK_F8: return CLIENT_KEY_F8;
    case SDLK_F9: return CLIENT_KEY_F9;   case SDLK_F10: return CLIENT_KEY_F10;
    case SDLK_F11: return CLIENT_KEY_F11; case SDLK_F12: return CLIENT_KEY_F12;
    case SDLK_BACKSPACE: return CLIENT_KEY_BACKSPACE;
    case SDLK_DELETE: return CLIENT_KEY_DELETE;
    case SDLK_TAB: return CLIENT_KEY_TAB;
    case SDLK_RETURN: return CLIENT_KEY_RETURN;
    case SDLK_KP_ENTER: return CLIENT_KEY_KP_ENTER;
    default: return CLIENT_KEY_NONE;
    }
}

static int sdl_client_button(Uint8 button) {
    if (button == SDL_BUTTON_LEFT) return CLIENT_BUTTON_LEFT;
    if (button == SDL_BUTTON_RIGHT) return CLIENT_BUTTON_RIGHT;
    if (button == SDL_BUTTON_MIDDLE) return CLIENT_BUTTON_MIDDLE;
    return CLIENT_BUTTON_NONE;
}

static bool sdl_client_event(const SDL_Event *in, ClientEvent *out) {
    memset(out, 0, sizeof(*out));
    switch (in->type) {
    case SDL_QUIT: out->type = CLIENT_EVENT_QUIT; return true;
    case SDL_MOUSEMOTION: out->type = CLIENT_EVENT_MOUSE_MOVE; out->x = in->motion.x; out->y = in->motion.y; return true;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        out->type = in->type == SDL_MOUSEBUTTONDOWN ? CLIENT_EVENT_MOUSE_DOWN : CLIENT_EVENT_MOUSE_UP;
        out->x = in->button.x; out->y = in->button.y; out->button = sdl_client_button(in->button.button);
        return true;
    case SDL_MOUSEWHEEL: out->type = CLIENT_EVENT_MOUSE_WHEEL; out->wheel = in->wheel.y; return true;
    case SDL_KEYUP: out->type = CLIENT_EVENT_KEY_UP; out->key = sdl_client_key(in->key.keysym.sym); return true;
    case SDL_KEYDOWN: out->type = CLIENT_EVENT_KEY_DOWN; out->key = sdl_client_key(in->key.keysym.sym); return true;
    case SDL_TEXTINPUT:
        out->type = CLIENT_EVENT_TEXT;
        memcpy(out->text, in->text.text, sizeof(out->text));
        out->text[sizeof(out->text) - 1] = '\0';
        return true;
    default: return false;
    }
}

#ifdef __WIIU__
#include <SDL2/SDL_syswm.h>
#include <sys/stat.h>
// The .wuhb bundles engine/data/pack/client under /vol/content; the client's
// "engine/data/pack/client/..." fallback paths then resolve from there.
#define WIIU_CONTENT_DIR "fs:/vol/content"
// Optional "host [port]" for the LAN server, e.g. "192.168.1.20 43595".
#define WIIU_APP_DIR "fs:/vol/external01/wiiu/apps/rs2004c"
#define WIIU_SERVER_FILE WIIU_APP_DIR "/server.txt"
#define WIIU_LOG_FILE WIIU_APP_DIR "/log.txt"
#define WIIU_ERR_FILE WIIU_APP_DIR "/errors.txt"
static char wiiu_server_host[64];

// System keyboard (swkbd). It opens when a login field or prompt becomes active,
// on a tap on a login field, or on a tap in the chatbox for public chat. OK
// replaces the field's text with the keyboard's: backspaces, the typed text
// (SDL text events), then Return for fields where Enter submits/advances. The
// original's Enter in the password field only refocuses the username, so the
// player taps "Login" there.
static ClientTextTarget wiiu_kbd_last = CLIENT_TEXT_NONE;
static ClientTextTarget wiiu_kbd_target = CLIENT_TEXT_NONE;
static bool wiiu_kbd_shown, wiiu_kbd_pending, wiiu_chat_tapped;
static int wiiu_kbd_hidden_frames;

static void wiiu_queue_key(int key) {
    ClientEvent e;
    memset(&e, 0, sizeof(e));
    e.type = CLIENT_EVENT_KEY_DOWN; e.key = key;
    client_log_event(&e); client_queue_event(&e);
    e.type = CLIENT_EVENT_KEY_UP;
    client_log_event(&e); client_queue_event(&e);
}

static void wiiu_keyboard_open(const ClientTextField *field) {
    static const char *hints[] = { "", "Username", "Password", "Chat", "Enter name or message", "Enter amount", "Enter name" };
    SDL_WiiUSetSWKBDInitialText(field->text);
    SDL_WiiUSetSWKBDHintText(hints[field->target]);
    SDL_WiiUSetSWKBDShowWordSuggestions(SDL_FALSE);
    if (field->target == CLIENT_TEXT_AMOUNT) SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_NUMPAD);
    if (field->target == CLIENT_TEXT_PASSWORD) SDL_WiiUSetSWKBDPasswordMode(SDL_WIIU_SWKBD_PASSWORD_MODE_FADE);
    wiiu_kbd_target = field->target;
    wiiu_kbd_shown = true;
    wiiu_kbd_hidden_frames = 0;
    SDL_StartTextInput();
}

static void wiiu_keyboard_closed(void) {
    SDL_StopTextInput();
    wiiu_kbd_shown = false;
}

// Handles swkbd OK/cancel notifications; returns true when the event was one.
static bool wiiu_keyboard_event(const SDL_Event *event) {
    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.x >= 22 && event->button.x < 501 &&
        event->button.y >= 375 && event->button.y < 471) wiiu_chat_tapped = true;
    if (event->type != SDL_SYSWMEVENT || event->syswm.msg->subsystem != SDL_SYSWM_WIIU) return false;
    unsigned what = event->syswm.msg->msg.wiiu.event;
    // Cemu keeps IsDecideOkButton true after OK until the next appear, so SDL
    // repeats OK_START/OK_FINISH on every pump; only the first one, while our
    // keyboard is open, is acted on.
    if (!wiiu_kbd_shown) return true;
    if (what == SDL_WIIU_SYSWM_SWKBD_OK_START_EVENT) {
        ClientTextField field = client_text_field();
        if (field.target == wiiu_kbd_target)
            for (size_t i = strlen(field.text); i > 0; i--) wiiu_queue_key(CLIENT_KEY_BACKSPACE);
    } else if (what == SDL_WIIU_SYSWM_SWKBD_OK_FINISH_EVENT) {
        if (wiiu_kbd_target != CLIENT_TEXT_PASSWORD && wiiu_kbd_target != CLIENT_TEXT_NONE) wiiu_queue_key(CLIENT_KEY_RETURN);
        wiiu_keyboard_closed();
    } else if (what == SDL_WIIU_SYSWM_SWKBD_CANCEL_EVENT) {
        wiiu_keyboard_closed();
    }
    return true;
}

// After a frame's updates: open the keyboard for a newly active field or a tap.
// On hardware the keyboard fades in and out over several frames and an appear
// issued during the fade-out is dropped (Cemu has no fade), so a requested open
// waits until the previous keyboard is hidden, and a keyboard of ours that has
// not been visible for a second is treated as closed.
static void wiiu_keyboard_frame(SDL_Window *window) {
    ClientTextField field = client_text_field();
    bool visible = SDL_IsScreenKeyboardShown(window);
    if (wiiu_kbd_shown) {
        wiiu_kbd_hidden_frames = visible ? 0 : wiiu_kbd_hidden_frames + 1;
        if (wiiu_kbd_hidden_frames > 50) wiiu_keyboard_closed();
    }
    if (!wiiu_kbd_shown) {
        if (field.target != wiiu_kbd_last && field.target != CLIENT_TEXT_NONE && field.target != CLIENT_TEXT_CHAT) wiiu_kbd_pending = true;
        if (field.tapped && (field.target == CLIENT_TEXT_USERNAME || field.target == CLIENT_TEXT_PASSWORD)) wiiu_kbd_pending = true;
        if (wiiu_chat_tapped && field.target == CLIENT_TEXT_CHAT) wiiu_kbd_pending = true;
    }
    if (field.target == CLIENT_TEXT_NONE) wiiu_kbd_pending = false;
    wiiu_kbd_last = field.target;
    wiiu_chat_tapped = false;
    if (wiiu_kbd_pending && !wiiu_kbd_shown && !visible) {
        wiiu_kbd_pending = false;
        wiiu_keyboard_open(&field);
    }
}
static void wiiu_read_server(ClientOptions *options) {
    // No argv on the console: default to start_local_server.bat's port, like play_client.bat.
    options->server_port = 43595;
    FILE *file = fopen(WIIU_SERVER_FILE, "r");
    if (!file) return;
    int port = 0;
    if (fscanf(file, "%63s %d", wiiu_server_host, &port) >= 1) {
        options->server_host = wiiu_server_host;
        if (port > 0 && port <= 65535) options->server_port = port;
    }
    fclose(file);
}
// Test login: an optional "autologin.txt" ("user password") next to server.txt
// logs in like RS2_USERNAME/RS2_PASSWORD on PC. Absent by default.
static char wiiu_login_user[16], wiiu_login_pass[24];
static void wiiu_read_autologin(ClientOptions *options) {
    FILE *file = fopen(WIIU_APP_DIR "/autologin.txt", "r");
    if (!file) return;
    if (fscanf(file, "%15s %23s", wiiu_login_user, wiiu_login_pass) == 2) {
        options->login_username = wiiu_login_user;
        options->login_password = wiiu_login_pass;
    }
    fclose(file);
}
#endif

#ifdef __WIIU__
#include <coreinit/thread.h>
#include <malloc.h>
#include <unistd.h>
int fileno(FILE *stream); // POSIX; hidden by -std=c99 in newlib's stdio.h
// Exit-path progress, synced to the SD card at once so a hang leaves its last
// step in log.txt even after the console is powered off.
static void wiiu_stage(const char *step) {
    printf("[WIIU] %s\n", step);
    fflush(stdout);
    fsync(fileno(stdout));
}
#define WIIU_STAGE(step) wiiu_stage(step)
static int client_main(int argc, char *argv[]);
static int wiiu_argc;
static char **wiiu_argv;
static int wiiu_client_thread(int argc, const char **argv) {
    (void)argc; (void)argv;
    return client_main(wiiu_argc, wiiu_argv);
}
// The PC build links a 16 MB stack (Makefile --stack); an RPX main thread has
// 64 KB, which the scene renderer's per-model scratch arrays overflow. Run the
// client on a main-core thread with the same 16 MB stack.
int main(int argc, char *argv[]) {
    enum { WIIU_STACK_SIZE = 16 * 1024 * 1024 };
    static OSThread thread __attribute__((aligned(16)));
    uint8_t *stack = memalign(16, WIIU_STACK_SIZE);
    if (!stack) return 1;
    wiiu_argc = argc; wiiu_argv = argv;
    if (!OSCreateThread(&thread, wiiu_client_thread, 0, NULL, stack + WIIU_STACK_SIZE, WIIU_STACK_SIZE,
                        OSGetThreadPriority(OSGetCurrentThread()), OS_THREAD_ATTRIB_AFFINITY_CPU1)) {
        free(stack); return 1;
    }
    OSResumeThread(&thread);
    int result = 1;
    OSJoinThread(&thread, &result);
    WIIU_STAGE("client thread joined");
    free(stack);
    return result;
}
static int client_main(int argc, char *argv[]) {
#else
#define WIIU_STAGE(step) ((void)0)
int main(int argc, char *argv[]) {
#endif
    ClientOptions options;
    if (!client_parse_args(argc, argv, &options)) return 1;
    setbuf(stdout, NULL);

#ifdef __WIIU__
    // The console has no visible stdout; keep the client's log on the SD card.
    mkdir("fs:/vol/external01/wiiu", 0777); mkdir("fs:/vol/external01/wiiu/apps", 0777); mkdir(WIIU_APP_DIR, 0777);
    if (freopen(WIIU_LOG_FILE, "w", stdout)) { setvbuf(stdout, NULL, _IOLBF, 0); }
    if (freopen(WIIU_ERR_FILE, "w", stderr)) setvbuf(stderr, NULL, _IOLBF, 0);
    wiiu_read_server(&options);
    wiiu_read_autologin(&options);
    if (chdir(WIIU_CONTENT_DIR) != 0) { perror("Cannot enter content directory"); return 1; }
#else
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
#endif

    if (options.record_path) {
        // A recording replays only from a known Math.random() seed.
        if (!options.has_seed) { options.has_seed = true; options.seed = (int64_t)(SDL_GetPerformanceCounter() & 0x7FFFFFFFFFFFLL); }
        if (!session_log_record(options.record_path)) { fprintf(stderr, "Cannot write session log %s\n", options.record_path); return 1; }
        client_log_write_header(argc, argv, options.seed, options.login_username);
        printf("[RECORD] session log -> %s (seed %lld)\n", options.record_path, (long long)options.seed);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    audio_init();
    (void)audio_start_worker225();
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

#ifdef __WIIU__
    // Letterbox the 765x503 frame on the TV/GamePad; SDL maps touch back to it.
    SDL_RenderSetLogicalSize(renderer, CLIENT_WIDTH, CLIENT_HEIGHT);
    // SDL's Wii U port reads GamePad touch (and feeds swkbd) only while the GamePad
    // joystick is open; touches then arrive as synthesized left-button mouse events.
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0) {
        for (int i = 0; i < SDL_NumJoysticks(); i++) (void)SDL_JoystickOpen(i);
    } else {
        fprintf(stderr, "SDL joystick init failed: %s\n", SDL_GetError());
    }
#endif
    SDL_Texture *framebuffer = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        CLIENT_WIDTH,
        CLIENT_HEIGHT
    );

    recovery_renderer=renderer; recovery_texture=framebuffer;
    ClientPlatform platform = { sdl_now_ms, sdl_save_bmp, sdl_present_recovery,sdl_tracking_now_ms };
    if (!client_init(&options, &platform)) { SDL_Quit(); return 1; }
#ifdef __WIIU__
    // swkbd OK/cancel arrive as SDL_SYSWMEVENT; the keyboard opens only while typing.
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#else
    SDL_StartTextInput();
#endif

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

    while (client_running()) {
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
        if (options.fixed_step) var4 = 256;
        else SDL_Delay(var5);

        client_log_frame(client_rendered_frames());
        client_begin_frame();
        SDL_Event event;
#ifdef __WIIU__
        // One pump per frame: SDL_PollEvent pumps on every call, and the repeated
        // swkbd OK events above would then never let the queue drain.
        SDL_PumpEvents();
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
#else
        while (SDL_PollEvent(&event)) {
#endif
            ClientEvent input;
#ifdef __WIIU__
            // Closing from the HOME menu: ProcUI waits for the app to exit, so stop
            // here rather than relying on the queued quit event being processed.
            if (event.type == SDL_QUIT) { WIIU_STAGE("quit requested"); client_stop(); }
            // HOME menu: background apps keep only core-2 threads, and SDL's audio
            // threads may run there; close audio before the foreground is released
            // (the next pump) and reopen it on return.
            if (event.type == SDL_APP_WILLENTERBACKGROUND) { audio_suspend(); WIIU_STAGE("audio suspended"); }
            if (event.type == SDL_APP_DIDENTERBACKGROUND) WIIU_STAGE("entered background");
            if (event.type == SDL_APP_DIDENTERFOREGROUND) { WIIU_STAGE("entered foreground"); audio_resume(); }
            if (wiiu_keyboard_event(&event)) continue;
            if (event.type == SDL_JOYDEVICEADDED) { (void)SDL_JoystickOpen(event.jdevice.which); continue; }
#endif
            if(tracking_sdl_event(&event,&input)){client_log_event(&input);client_queue_event(&input);}
            if (sdl_client_event(&event, &input)) { client_log_event(&input); client_queue_event(&input); }
        }
#ifdef __WIIU__
        // Closing from the HOME menu arrives while still in the background, where
        // the rest of the frame (swkbd state, game update, drawing) can block: the
        // October 27 hardware log ends at "quit requested". Shut down at once.
        if (!client_running()) break;
#endif
        const uint8_t *keystates = SDL_GetKeyboardState(NULL);
        ClientHeldKeys held = {
            .left = keystates[SDL_SCANCODE_LEFT], .right = keystates[SDL_SCANCODE_RIGHT],
            .up = keystates[SDL_SCANCODE_UP], .down = keystates[SDL_SCANCODE_DOWN],
            .w = keystates[SDL_SCANCODE_W], .a = keystates[SDL_SCANCODE_A],
            .s = keystates[SDL_SCANCODE_S], .d = keystates[SDL_SCANCODE_D],
            .ctrl = (SDL_GetModState() & KMOD_CTRL) != 0
        };
        client_log_held(&held);
        client_set_held_keys(&held);
        client_process_events();

        // 50 Hz Client Logic Tick Accumulator (matching GameShell.java:200-206)
        while (var6 < 256) {
            client_log_update();
            if (!client_update()) break;
            var6 += var4;
        }
        var6 &= 0xFF;
#ifdef __WIIU__
        wiiu_keyboard_frame(window);
#endif

        if (!client_draw()) return 1;
        if (session_log_mode() == SESSION_LOG_RECORD) client_log_draw(client_frame_hash());

        // Update SDL texture & present
        SDL_UpdateTexture(framebuffer, NULL, client_pixels(), CLIENT_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
        const char *capture = client_frame_capture_path();
        if (capture) {
            SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, CLIENT_WIDTH, CLIENT_HEIGHT, 32, SDL_PIXELFORMAT_RGB888);
            if (!surface || SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) != 0 ||
                SDL_SaveBMP(surface, capture) != 0) {
                fprintf(stderr, "Client capture failed: %s\n", SDL_GetError());
                return 1;
            }
            SDL_FreeSurface(surface);
            printf("[CAPTURE] SDL renderer -> %s\n", capture);
        }
        SDL_RenderPresent(renderer);
        client_end_frame();
    }

    WIIU_STAGE("main loop ended");
    audio_close();
    WIIU_STAGE("audio closed");
    midi_close();
    client_shutdown();
    WIIU_STAGE("client shut down");
    // Marks a complete recording; a log cut at a frame boundary lacks it.
    if (session_log_mode() == SESSION_LOG_RECORD) { session_log_begin("Q"); session_log_end(); }
    session_log_close();
#ifdef __WIIU__
    if (wiiu_kbd_shown) SDL_StopTextInput(); // no swkbd calls in the background unless open
#else
    SDL_StopTextInput();
#endif
    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    WIIU_STAGE("SDL_Quit");
    SDL_Quit();

    printf("[RS2004-C] Shutdown complete.\n");
    return 0;
}
