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

int main(int argc, char *argv[]) {
    ClientOptions options;
    if (!client_parse_args(argc, argv, &options)) return 1;
    setbuf(stdout, NULL);

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
        while (SDL_PollEvent(&event)) {
            ClientEvent input;
            if(tracking_sdl_event(&event,&input)){client_log_event(&input);client_queue_event(&input);}
            if (sdl_client_event(&event, &input)) { client_log_event(&input); client_queue_event(&input); }
        }
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

    audio_close();
    midi_close();
    client_shutdown();
    // Marks a complete recording; a log cut at a frame boundary lacks it.
    if (session_log_mode() == SESSION_LOG_RECORD) { session_log_begin("Q"); session_log_end(); }
    session_log_close();
    SDL_StopTextInput();
    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("[RS2004-C] Shutdown complete.\n");
    return 0;
}
