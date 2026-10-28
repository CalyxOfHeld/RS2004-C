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
#ifdef __WIIU__
#include "dash3d/scene_runtime225.h"
#include <coreinit/core.h>
#endif
#include "audio/midi.h"

// The client frame is 0x00RRGGBB per pixel (SDL RGB888). The Wii U GX2 renderer
// has no RGB888 texture, so SDL converted every frame through its generic blitter
// (19 ms, October 28 hardware log), and ARGB8888 drew pixels with a zero top byte
// black. There the texture is RGBX8888 (0xRRGGBBxx), filled by one shift per pixel.
#ifdef __WIIU__
#define SDL_FRAME_FORMAT SDL_PIXELFORMAT_RGBX8888
#else
#define SDL_FRAME_FORMAT SDL_PIXELFORMAT_RGB888
#endif
#ifdef __WIIU__
static uint64_t wiiu_shift_sum;
#endif
static void sdl_upload_frame(SDL_Texture *texture, const uint32_t *pixels, int width, int height) {
#ifdef __WIIU__
    // Shifting straight into the locked texture took 17-22 ms a frame (that
    // memory appears uncached). Shift into a cached buffer instead, allocating
    // each destination line with dcbz so it is not read first, then hand the
    // matching RGBX8888 rows to SDL_UpdateTexture. Times are logged as shift
    // and the rest of upload.
    static uint32_t shifted[CLIENT_WIDTH * CLIENT_HEIGHT] __attribute__((aligned(32)));
    uint64_t start = SDL_GetPerformanceCounter();
    size_t count = (size_t)width * (size_t)height, i = 0;
    if (count > sizeof shifted / sizeof shifted[0]) return;
    for (; i + 8 <= count; i += 8) {
        __asm__ volatile("dcbz 0,%0" : : "r"(shifted + i) : "memory");
        uint32_t a = pixels[i], b = pixels[i + 1], c = pixels[i + 2], d = pixels[i + 3];
        uint32_t e = pixels[i + 4], f = pixels[i + 5], g = pixels[i + 6], h = pixels[i + 7];
        shifted[i] = a << 8; shifted[i + 1] = b << 8; shifted[i + 2] = c << 8; shifted[i + 3] = d << 8;
        shifted[i + 4] = e << 8; shifted[i + 5] = f << 8; shifted[i + 6] = g << 8; shifted[i + 7] = h << 8;
    }
    for (; i < count; i++) shifted[i] = pixels[i] << 8;
    wiiu_shift_sum += SDL_GetPerformanceCounter() - start;
    SDL_UpdateTexture(texture, NULL, shifted, width * sizeof(uint32_t));
#else
    (void)height;
    SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(uint32_t));
#endif
}

static SDL_Renderer *recovery_renderer;
static SDL_Texture *recovery_texture;
static void sdl_present_recovery(const uint32_t *pixels, int width, int height) {
    sdl_upload_frame(recovery_texture, pixels, width, height);
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
static bool wiiu_background;
static SDL_Window *wiiu_kbd_window; // for the [INPUT] keyboard state
// Frame-phase timing for hardware profiling: averages every 5 s to log.txt.
// Draw is split by scene_perf_mark225 (client.c, scene_runtime225.c): pre (camera,
// setup), sync (scene clear, world sync, ground objects), actors (entity models), prepare (visibility), raster (scene traversal and
// rasterising), overlay (viewport copy, picking, overheads, menus); "ui" is the rest
// of the draw (shell, minimap, panels, text), or all of it on the title screen.
enum { WIIU_T_SLEEP, WIIU_T_INPUT, WIIU_T_UPDATE, WIIU_T_PRE, WIIU_T_SYNC, WIIU_T_ACTORS, WIIU_T_PREPARE,
       WIIU_T_RASTER, WIIU_T_OVERLAY, WIIU_T_DRAW, WIIU_T_UPLOAD, WIIU_T_PRESENT, WIIU_T_COUNT };
static uint64_t wiiu_t_sum[WIIU_T_COUNT], wiiu_t_last, wiiu_t_window;
static int wiiu_t_frames, wiiu_t_ticks;
// Input diagnostics per window: finger and swkbd (SDL_SYSWMEVENT) events seen.
static int wiiu_n_finger, wiiu_n_syswm;
static void wiiu_time(int phase) {
    uint64_t now = SDL_GetPerformanceCounter();
    wiiu_t_sum[phase] += now - wiiu_t_last;
    wiiu_t_last = now;
}
static void wiiu_time_frame(void) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    wiiu_t_frames++;
    if (wiiu_t_window == 0) wiiu_t_window = wiiu_t_last;
    if (wiiu_t_last - wiiu_t_window < freq * 5) return;
    double ms = 1000.0 / (double)freq / wiiu_t_frames, secs = (double)(wiiu_t_last - wiiu_t_window) / (double)freq;
    printf("[PERF] fps=%.1f ticks/s=%.1f ms: sleep=%.1f input=%.1f update=%.1f | pre=%.1f sync=%.1f actors=%.1f prepare=%.1f "
        "raster=%.1f overlay=%.1f ui=%.1f | upload=%.1f (shift=%.1f) present=%.1f\n",
        wiiu_t_frames / secs, wiiu_t_ticks / secs, wiiu_t_sum[0] * ms, wiiu_t_sum[1] * ms, wiiu_t_sum[2] * ms,
        wiiu_t_sum[3] * ms, wiiu_t_sum[4] * ms, wiiu_t_sum[5] * ms, wiiu_t_sum[6] * ms, wiiu_t_sum[7] * ms,
        wiiu_t_sum[8] * ms, wiiu_t_sum[9] * ms, wiiu_t_sum[10] * ms, wiiu_shift_sum * ms, wiiu_t_sum[11] * ms);
    printf("[INPUT] fingers=%d syswm=%d kbd_shown=%d kbd_visible=%d text_input=%d target=%d\n",
        wiiu_n_finger, wiiu_n_syswm, wiiu_kbd_shown, (wiiu_kbd_window && SDL_IsScreenKeyboardShown(wiiu_kbd_window)) ? 1 : 0,
        SDL_IsTextInputActive() ? 1 : 0, (int)client_text_field().target);
    wiiu_n_finger = wiiu_n_syswm = 0;
    fflush(stdout);
    memset(wiiu_t_sum, 0, sizeof wiiu_t_sum); wiiu_shift_sum = 0;
    wiiu_t_frames = wiiu_t_ticks = 0;
    wiiu_t_window = wiiu_t_last;
}
#define WIIU_TIME(phase) wiiu_time(phase)
// Sampling profiler: a 1 kHz periodic alarm on the client's core records the
// interrupted PC and LR while in game (from the first scene draw) until the
// buffer is full; samples.bin is written at exit. The address of
// wiiu_profile_reference lets the PC map samples onto rs2004c.elf symbols.
#include <coreinit/alarm.h>
#include <coreinit/time.h>
enum { WIIU_PROFILE_MAX = 131072 };
static uint32_t wiiu_profile_samples[WIIU_PROFILE_MAX][2];
static volatile uint32_t wiiu_profile_count;
static volatile bool wiiu_profile_on;
static OSAlarm wiiu_profile_alarm;
void wiiu_profile_reference(void) {}
static void wiiu_profile_tick(OSAlarm *alarm, OSContext *context) {
    (void)alarm;
    if (!wiiu_profile_on || !context || wiiu_profile_count >= WIIU_PROFILE_MAX) return;
    wiiu_profile_samples[wiiu_profile_count][0] = context->srr0;
    wiiu_profile_samples[wiiu_profile_count][1] = context->lr;
    wiiu_profile_count++;
}
// Addresses of system library functions, so profile samples outside
// rs2004c.elf can be matched to the nearest export.
#include <coreinit/dynload.h>
static void wiiu_profile_exports(void) {
    static const char *const libs[] = { "coreinit.rpl", "gx2.rpl" };
    static const char *const names[2][24] = {
        { "OSBlockSet", "OSBlockMove", "memset", "memcpy", "memmove", "DCFlushRange", "DCStoreRange",
          "DCInvalidateRange", "DCZeroRange", "OSSleepTicks", "OSYieldThread", "OSWaitEvent", "OSLockMutex",
          "OSFastMutex_Lock", "OSUninterruptibleSpinLock_Acquire", "MEMAllocFromExpHeapEx", "MEMFreeToExpHeap",
          "MEMAllocFromFrmHeapEx", "OSGetTime", "OSGetSystemTime", "OSDisableInterrupts", "OSRestoreInterrupts",
          "OSSwitchSecCodeGenMode", NULL },
        { "GX2DrawDone", "GX2WaitForVsync", "GX2WaitForFlip", "GX2SwapScanBuffers", "GX2Flush", "GX2Invalidate",
          "GX2CopyColorBufferToScanBuffer", "GX2SetContextState", "GX2WaitTimeStamp", "GX2GetLastSubmittedTimeStamp",
          "GX2CopySurface", NULL } };
    for (int l = 0; l < 2; l++) {
        OSDynLoad_Module module;
        if (OSDynLoad_Acquire(libs[l], &module) != OS_DYNLOAD_OK) continue;
        for (int i = 0; names[l][i]; i++) {
            void *address = NULL;
            if (OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, names[l][i], &address) == OS_DYNLOAD_OK)
                printf("[EXPORT] %08x %s %s\n", (unsigned)(uintptr_t)address, libs[l], names[l][i]);
        }
        OSDynLoad_Release(module);
    }
    fflush(stdout);
}
static void wiiu_profile_start(void) {
    wiiu_profile_exports();
    OSCreateAlarm(&wiiu_profile_alarm);
    OSSetPeriodicAlarm(&wiiu_profile_alarm, OSMillisecondsToTicks(1), OSMicrosecondsToTicks(1000), wiiu_profile_tick);
}
static void wiiu_profile_stop(void) {
    wiiu_profile_on = false;
    OSCancelAlarm(&wiiu_profile_alarm);
    FILE *file = fopen(WIIU_APP_DIR "/samples.bin", "wb");
    if (!file) return;
    uint32_t header[3] = { 0x52505246u, (uint32_t)(uintptr_t)&wiiu_profile_reference, wiiu_profile_count };
    fwrite(header, sizeof header, 1, file);
    fwrite(wiiu_profile_samples, sizeof wiiu_profile_samples[0], wiiu_profile_count, file);
    fclose(file);
    printf("[PROFILE] %u samples written, reference=%08x\n", (unsigned)header[2], (unsigned)header[1]);
    fflush(stdout);
}
static void wiiu_scene_mark(int phase) { wiiu_profile_on = true; wiiu_time(WIIU_T_PRE + phase); }
// Startup check: memset/memcpy speed on frame-sized (765x503x4) buffers and a
// plain integer loop, to tell slow memory apart from slow code.
static uint32_t wiiu_bench_static[CLIENT_WIDTH * CLIENT_HEIGHT];
static void wiiu_memory_benchmark(void) {
    const size_t bytes = sizeof(wiiu_bench_static);
    uint32_t *heap = malloc(bytes);
    if (!heap) return;
    double freq = (double)SDL_GetPerformanceFrequency();
    uint64_t t0 = SDL_GetPerformanceCounter();
    for (int i = 0; i < 10; i++) memset(wiiu_bench_static, i, bytes);
    uint64_t t1 = SDL_GetPerformanceCounter();
    volatile uint32_t sum = 0;
    // Read back after each pass: GCC drops stores to a buffer that is only freed.
    for (int i = 0; i < 10; i++) { memset(heap, i, bytes); sum += heap[i * 997]; }
    uint64_t t2 = SDL_GetPerformanceCounter();
    for (int i = 0; i < 10; i++) { memcpy(heap, wiiu_bench_static, bytes); sum += heap[i * 991]; }
    uint64_t t3 = SDL_GetPerformanceCounter();
    for (uint32_t i = 0; i < 10000000u; i++) sum += i ^ (sum >> 3);
    uint64_t t4 = SDL_GetPerformanceCounter();
    double mb = 10.0 * (double)bytes / (1024.0 * 1024.0);
    printf("[BENCH] memset static=%.0f MB/s heap=%.0f MB/s memcpy=%.0f MB/s; 10M int loop=%.1f ms; core=%d\n",
        mb / ((t1 - t0) / freq), mb / ((t2 - t1) / freq), mb / ((t3 - t2) / freq),
        (t4 - t3) * 1000.0 / freq, (int)OSGetCoreId());
    fflush(stdout);
    free(heap);
}

static void wiiu_queue_key(int key) {
    ClientEvent e;
    memset(&e, 0, sizeof(e));
    e.type = CLIENT_EVENT_KEY_DOWN; e.key = key;
    client_log_event(&e); client_queue_event(&e);
    e.type = CLIENT_EVENT_KEY_UP;
    client_log_event(&e); client_queue_event(&e);
}

// GamePad controls. The client receives the same mouse and arrow-key input a PC
// sends; SDL's own touch-to-mouse is off (main) so a touch can mean more than a
// left click:
//   tap                -> left click at the touch point (on release)
//   hold 500 ms        -> right click at the touch point (opens the menu)
//   drag               -> left-button drag (moving inventory items)
//   L or ZL held + tap -> right click at once
//   D-pad, right stick -> arrow keys (camera)
enum { WIIU_TOUCH_IDLE, WIIU_TOUCH_PENDING, WIIU_TOUCH_DRAG, WIIU_TOUCH_DONE };
enum { WIIU_PAD_L = 6, WIIU_PAD_ZL = 8, WIIU_PAD_LEFT = 12, WIIU_PAD_UP = 13, WIIU_PAD_RIGHT = 14,
       WIIU_PAD_DOWN = 15, WIIU_PAD_RSTICK_LEFT = 20 }; // SDL wiiu vpad_button_map order
static SDL_Joystick *wiiu_gamepad;
static int wiiu_touch_state, wiiu_touch_x, wiiu_touch_y;

// Touch diagnostics for hardware runs (log.txt; touches are infrequent).
static void wiiu_input_log(const char *what, int x, int y, int value) {
    printf("[WIIU] %s x=%d y=%d %d\n", what, x, y, value);
    fflush(stdout);
}
static uint64_t wiiu_touch_ms;
static bool wiiu_arrows[4];

static void wiiu_queue_mouse(ClientEventType type, int x, int y, int button) {
    ClientEvent e;
    memset(&e, 0, sizeof(e));
    e.type = type; e.x = x; e.y = y; e.button = button;
    client_log_event(&e); client_queue_event(&e);
}

// The client builds the menu (the action a click runs) while drawing, from the
// mouse position (client_draw), as the original does; a click resolves against
// the last drawn menu. A mouse rests over a button before clicking, but a tap
// moves and clicks at once, so the press waits for one frame drawn at the new
// position.
static int wiiu_click_x, wiiu_click_y, wiiu_click_button, wiiu_click_wait = -1;

static void wiiu_click_press(void) {
    wiiu_input_log("press", wiiu_click_x, wiiu_click_y, wiiu_click_button);
    wiiu_queue_mouse(CLIENT_EVENT_MOUSE_DOWN, wiiu_click_x, wiiu_click_y, wiiu_click_button);
    wiiu_queue_mouse(CLIENT_EVENT_MOUSE_UP, wiiu_click_x, wiiu_click_y, wiiu_click_button);
    wiiu_click_wait = -1;
}

static void wiiu_click(int x, int y, int button) {
    if (wiiu_click_wait >= 0) wiiu_click_press();
    // The chatbox tap opens the keyboard for public chat (wiiu_keyboard_frame).
    if (button == CLIENT_BUTTON_LEFT && x >= 22 && x < 501 && y >= 375 && y < 471) wiiu_chat_tapped = true;
    wiiu_queue_mouse(CLIENT_EVENT_MOUSE_MOVE, x, y, CLIENT_BUTTON_NONE);
    wiiu_click_x = x; wiiu_click_y = y; wiiu_click_button = button; wiiu_click_wait = 1;
}

static bool wiiu_pad_button(int button) {
    return wiiu_gamepad && SDL_JoystickGetButton(wiiu_gamepad, button);
}

static bool wiiu_touch_event(const SDL_Event *event) {
    if (event->type != SDL_FINGERDOWN && event->type != SDL_FINGERMOTION && event->type != SDL_FINGERUP) return false;
    wiiu_n_finger++;
    int x = (int)(event->tfinger.x * CLIENT_WIDTH), y = (int)(event->tfinger.y * CLIENT_HEIGHT);
    if (x >= CLIENT_WIDTH) x = CLIENT_WIDTH - 1;
    if (y >= CLIENT_HEIGHT) y = CLIENT_HEIGHT - 1;
    if (event->type == SDL_FINGERDOWN) {
        wiiu_input_log("touch down", x, y, wiiu_pad_button(WIIU_PAD_L) || wiiu_pad_button(WIIU_PAD_ZL));
        if (wiiu_pad_button(WIIU_PAD_L) || wiiu_pad_button(WIIU_PAD_ZL)) {
            wiiu_click(x, y, CLIENT_BUTTON_RIGHT);
            wiiu_touch_state = WIIU_TOUCH_DONE;
        } else {
            wiiu_touch_state = WIIU_TOUCH_PENDING;
            wiiu_touch_x = x; wiiu_touch_y = y; wiiu_touch_ms = SDL_GetTicks64();
            wiiu_queue_mouse(CLIENT_EVENT_MOUSE_MOVE, x, y, CLIENT_BUTTON_NONE);
        }
    } else if (event->type == SDL_FINGERMOTION) {
        if (wiiu_touch_state == WIIU_TOUCH_PENDING && abs(x - wiiu_touch_x) + abs(y - wiiu_touch_y) > 8) {
            wiiu_queue_mouse(CLIENT_EVENT_MOUSE_DOWN, wiiu_touch_x, wiiu_touch_y, CLIENT_BUTTON_LEFT);
            wiiu_input_log("drag start", x, y, 0);
            wiiu_touch_state = WIIU_TOUCH_DRAG;
        }
        if (wiiu_touch_state == WIIU_TOUCH_DRAG) wiiu_queue_mouse(CLIENT_EVENT_MOUSE_MOVE, x, y, CLIENT_BUTTON_NONE);
    } else {
        wiiu_input_log("touch up", x, y, wiiu_touch_state);
        if (wiiu_touch_state == WIIU_TOUCH_PENDING) wiiu_click(wiiu_touch_x, wiiu_touch_y, CLIENT_BUTTON_LEFT);
        if (wiiu_touch_state == WIIU_TOUCH_DRAG) {
            wiiu_queue_mouse(CLIENT_EVENT_MOUSE_MOVE, x, y, CLIENT_BUTTON_NONE);
            wiiu_queue_mouse(CLIENT_EVENT_MOUSE_UP, x, y, CLIENT_BUTTON_LEFT);
        }
        wiiu_touch_state = WIIU_TOUCH_IDLE;
    }
    return true;
}

// Once per frame after events: long-press right click and GamePad camera keys.
static void wiiu_pad_frame(ClientHeldKeys *held) {
    // Before this frame's events are processed: wait 1 lets the move be processed
    // and drawn this frame; wait 0 presses on the next.
    if (wiiu_click_wait == 0) wiiu_click_press();
    else if (wiiu_click_wait > 0) wiiu_click_wait--;
    if (wiiu_touch_state == WIIU_TOUCH_PENDING && SDL_GetTicks64() - wiiu_touch_ms >= 500) {
        wiiu_click(wiiu_touch_x, wiiu_touch_y, CLIENT_BUTTON_RIGHT);
        wiiu_touch_state = WIIU_TOUCH_DONE;
    }
    // Left, right, up, down: D-pad or right stick, as the arrow keys' held flags
    // plus the key presses input tracking records (GameShell codes 1-4).
    static const int dpad[4] = { WIIU_PAD_LEFT, WIIU_PAD_RIGHT, WIIU_PAD_UP, WIIU_PAD_DOWN };
    static const int rstick[4] = { WIIU_PAD_RSTICK_LEFT, WIIU_PAD_RSTICK_LEFT + 2, WIIU_PAD_RSTICK_LEFT + 1, WIIU_PAD_RSTICK_LEFT + 3 };
    for (int i = 0; i < 4; i++) {
        bool down = wiiu_pad_button(dpad[i]) || wiiu_pad_button(rstick[i]);
        if (down != wiiu_arrows[i]) {
            ClientEvent e;
            memset(&e, 0, sizeof(e));
            e.type = down ? CLIENT_EVENT_TRACK_KEY_DOWN : CLIENT_EVENT_TRACK_KEY_UP; e.key = i + 1;
            client_log_event(&e); client_queue_event(&e);
            wiiu_arrows[i] = down;
        }
    }
    held->left |= wiiu_arrows[0]; held->right |= wiiu_arrows[1];
    held->up |= wiiu_arrows[2]; held->down |= wiiu_arrows[3];
}

static void wiiu_open_joystick(int index) {
    SDL_Joystick *joystick = SDL_JoystickOpen(index);
    const char *name = joystick ? SDL_JoystickName(joystick) : NULL;
    wiiu_input_log(name ? name : "joystick open failed", index, 0, joystick != NULL);
    if (name && !strcmp(name, "WiiU Gamepad")) wiiu_gamepad = joystick;
}

static void wiiu_keyboard_open(const ClientTextField *field) {
    static const char *hints[] = { "", "Username", "Password", "Chat", "Enter name or message", "Enter amount", "Enter name" };
    SDL_WiiUSetSWKBDInitialText(field->text);
    SDL_WiiUSetSWKBDHintText(hints[field->target]);
    SDL_WiiUSetSWKBDShowWordSuggestions(SDL_FALSE);
    if (field->target == CLIENT_TEXT_AMOUNT) SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_NUMPAD);
    if (field->target == CLIENT_TEXT_PASSWORD) SDL_WiiUSetSWKBDPasswordMode(SDL_WIIU_SWKBD_PASSWORD_MODE_FADE);
    wiiu_kbd_target = field->target;
    wiiu_input_log("keyboard open", (int)field->target, 0, 1);
    wiiu_kbd_shown = true;
    wiiu_kbd_hidden_frames = 0;
    SDL_StartTextInput();
}

static void wiiu_keyboard_closed(void) {
    wiiu_input_log("keyboard closed", (int)wiiu_kbd_target, 0, wiiu_kbd_hidden_frames);
    SDL_StopTextInput();
    wiiu_kbd_shown = false;
}

// Handles swkbd OK/cancel notifications; returns true when the event was one.
static bool wiiu_keyboard_event(const SDL_Event *event) {
    if (event->type != SDL_SYSWMEVENT || event->syswm.msg->subsystem != SDL_SYSWM_WIIU) return false;
    wiiu_n_syswm++;
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
    wiiu_kbd_window = window;
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
#ifndef __WIIU__
#define WIIU_TIME(phase) ((void)0)
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
static int wiiu_argc, wiiu_result = 1;
static char **wiiu_argv;
static void wiiu_client_entry(void) {
    wiiu_result = client_main(wiiu_argc, wiiu_argv);
}
// Calls fn with the stack pointer moved to stack_top (r3 = fn, r4 = stack_top)
// and returns on the original stack. The new frame's back chain links to the
// old one.
void wiiu_call_on_stack(void (*fn)(void), void *stack_top);
__asm__(
    "    .text\n"
    "    .align 2\n"
    "    .type wiiu_call_on_stack, @function\n"
    "wiiu_call_on_stack:\n"
    "    mflr 0\n"
    "    stw 0, 4(1)\n"
    "    stwu 1, -16(1)\n"
    "    clrrwi 4, 4, 4\n"
    "    stwu 1, -16(4)\n"
    "    mr 1, 4\n"
    "    mtctr 3\n"
    "    bctrl\n"
    "    lwz 1, 0(1)\n"
    "    addi 1, 1, 16\n"
    "    lwz 0, 4(1)\n"
    "    mtlr 0\n"
    "    blr\n"
    "    .size wiiu_call_on_stack, .-wiiu_call_on_stack\n");
// The PC build links a 16 MB stack (Makefile --stack); an RPX main thread has
// 64 KB (elf2rpl's fixed FILEINFO stackSize), which the scene renderer's
// per-model scratch arrays overflow. The client runs on the main thread itself
// with a 16 MB stack: a separate client thread driving ProcUI froze the
// console at the HOME menu on hardware (October 27), and the system suspends an
// app's other core 0/1 threads when it leaves the foreground. (Cemu 2.6 exits
// on coreinit's OSSwitchFiber, hence the explicit stack switch.)
int main(int argc, char *argv[]) {
    enum { WIIU_STACK_SIZE = 16 * 1024 * 1024 };
    uint8_t *stack = memalign(16, WIIU_STACK_SIZE);
    if (!stack) return 1;
    wiiu_argc = argc; wiiu_argv = argv;
    // Keep the thread's recorded stack bounds (and end-of-stack magic) on the
    // stack in use.
    OSThread *thread = OSGetCurrentThread();
    void *old_start = thread->stackStart, *old_end = thread->stackEnd;
    *(uint32_t *)stack = 0xDEADBABEu;
    thread->stackStart = stack + WIIU_STACK_SIZE; thread->stackEnd = stack;
    wiiu_call_on_stack(wiiu_client_entry, stack + WIIU_STACK_SIZE);
    thread->stackStart = old_start; thread->stackEnd = old_end;
    WIIU_STAGE("client finished");
    free(stack);
    return wiiu_result;
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

#ifdef __WIIU__
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0"); // touches are translated by wiiu_touch_event
#endif
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
    // joystick is open; touches arrive as finger events (wiiu_touch_event).
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0) {
        for (int i = 0; i < SDL_NumJoysticks(); i++) wiiu_open_joystick(i);
    } else {
        fprintf(stderr, "SDL joystick init failed: %s\n", SDL_GetError());
    }
#endif
    const Uint32 framebuffer_format = SDL_FRAME_FORMAT;
    SDL_Texture *framebuffer = SDL_CreateTexture(
        renderer,
        framebuffer_format,
        SDL_TEXTUREACCESS_STREAMING,
        CLIENT_WIDTH,
        CLIENT_HEIGHT
    );
#ifdef __WIIU__
    // Hardware profiling: upload took 20 ms per frame. Log what the GX2 renderer
    // offers, and alternate two textures so an update never waits for the GPU to
    // finish drawing the texture presented last frame.
    {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(renderer, &info) == 0) {
            printf("[WIIU] renderer %s flags=0x%x max=%dx%d formats:", info.name, (unsigned)info.flags,
                info.max_texture_width, info.max_texture_height);
            for (Uint32 i = 0; i < info.num_texture_formats; i++)
                printf(" %s", SDL_GetPixelFormatName(info.texture_formats[i]));
            printf("\n");
            fflush(stdout);
        }
    }
    SDL_Texture *wiiu_framebuffers[2] = { framebuffer,
        SDL_CreateTexture(renderer, framebuffer_format, SDL_TEXTUREACCESS_STREAMING, CLIENT_WIDTH, CLIENT_HEIGHT) };
    for (int i = 0; i < 2; i++) if (wiiu_framebuffers[i]) SDL_SetTextureBlendMode(wiiu_framebuffers[i], SDL_BLENDMODE_NONE);
    wiiu_memory_benchmark();
    int wiiu_framebuffer_next = 0;
    scene_perf_mark225 = wiiu_scene_mark;
    wiiu_profile_start();
#endif

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
#ifdef __WIIU__
    wiiu_t_last = SDL_GetPerformanceCounter();
#endif

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
        WIIU_TIME(WIIU_T_SLEEP);

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
            if (event.type == SDL_APP_DIDENTERBACKGROUND) { WIIU_STAGE("entered background"); wiiu_background = true; }
            if (event.type == SDL_APP_DIDENTERFOREGROUND) { WIIU_STAGE("entered foreground"); audio_resume(); }
            if (wiiu_keyboard_event(&event)) continue;
            if (event.type == SDL_JOYDEVICEADDED) { wiiu_open_joystick(event.jdevice.which); continue; }
            if (event.type == SDL_JOYDEVICEREMOVED) {
                if (wiiu_gamepad && SDL_JoystickInstanceID(wiiu_gamepad) == event.jdevice.which) wiiu_gamepad = NULL;
                continue;
            }
            if (wiiu_touch_event(&event)) continue;
#endif
            if(tracking_sdl_event(&event,&input)){client_log_event(&input);client_queue_event(&input);}
            if (sdl_client_event(&event, &input)) { client_log_event(&input); client_queue_event(&input); }
        }
#ifdef __WIIU__
        // Closing from the HOME menu arrives while still in the background, where
        // the rest of the frame (swkbd state, game update, drawing) can block: the
        // October 27 hardware log ends at "quit requested". Shut down at once.
        if (!client_running()) break;
        // Same for the HOME menu: the foreground is released (ProcUIDrawDoneRelease)
        // only by the next pump, and in-game hardware logs end at "entered
        // background". Skip this frame's update, draw and present.
        if (wiiu_background) { wiiu_background = false; WIIU_STAGE("background frame skipped"); continue; }
#endif
        const uint8_t *keystates = SDL_GetKeyboardState(NULL);
        ClientHeldKeys held = {
            .left = keystates[SDL_SCANCODE_LEFT], .right = keystates[SDL_SCANCODE_RIGHT],
            .up = keystates[SDL_SCANCODE_UP], .down = keystates[SDL_SCANCODE_DOWN],
            .w = keystates[SDL_SCANCODE_W], .a = keystates[SDL_SCANCODE_A],
            .s = keystates[SDL_SCANCODE_S], .d = keystates[SDL_SCANCODE_D],
            .ctrl = (SDL_GetModState() & KMOD_CTRL) != 0
        };
#ifdef __WIIU__
        wiiu_pad_frame(&held);
#endif
        client_log_held(&held);
        client_set_held_keys(&held);
        client_process_events();
        WIIU_TIME(WIIU_T_INPUT);

        // 50 Hz Client Logic Tick Accumulator (matching GameShell.java:200-206)
        while (var6 < 256) {
            client_log_update();
            if (!client_update()) break;
#ifdef __WIIU__
            wiiu_t_ticks++;
#endif
            var6 += var4;
        }
        var6 &= 0xFF;
#ifdef __WIIU__
        wiiu_keyboard_frame(window);
#endif
        WIIU_TIME(WIIU_T_UPDATE);

        if (!client_draw()) return 1;
        if (session_log_mode() == SESSION_LOG_RECORD) client_log_draw(client_frame_hash());
        WIIU_TIME(WIIU_T_DRAW);

        // Update SDL texture & present
#ifdef __WIIU__
        if (wiiu_framebuffers[1]) { framebuffer = wiiu_framebuffers[wiiu_framebuffer_next]; wiiu_framebuffer_next ^= 1; }
#endif
        sdl_upload_frame(framebuffer, client_pixels(), CLIENT_WIDTH, CLIENT_HEIGHT);
        WIIU_TIME(WIIU_T_UPLOAD);
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
#ifdef __WIIU__
        wiiu_time(WIIU_T_PRESENT);
        wiiu_time_frame();
#endif
        client_end_frame();
    }

    WIIU_STAGE("main loop ended");
#ifdef __WIIU__
    wiiu_profile_stop();
#endif
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
